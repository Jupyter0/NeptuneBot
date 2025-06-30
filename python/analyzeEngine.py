import subprocess
import time
import os
import requests
from dotenv import load_dotenv

# === CONFIGURATION ===
bot1_env = ".env.botA"
bot2_env = ".env.botB"
bot1_username = "Neptune-Bot"
bot2_username = "Neptune-Backup"

load_dotenv()

bot1_token = os.getenv("BOT1_TOKEN")  # Or load from file
bot2_token = os.getenv("BOT2_TOKEN")

# === UTILITY ===
def launch_bot(env_file, username):
    return subprocess.Popen(["dotnet", "run", env_file, username])

def challenge_bot(from_token, to_username):
    print(f"[INFO] Sending challenge to {to_username}")
    res = requests.post(
        f"https://lichess.org/api/challenge/{to_username}",
        headers={
            "Authorization": f"Bearer {from_token}",
            "Content-Type": "application/x-www-form-urlencoded"
        },
        data={
            "rated": "false",
            "clock.limit": "60",
            "clock.increment": "1",
            "color": "white"
        }
    )
    if res.status_code == 200:
        print("[INFO] Challenge sent.")
        return res.json()['id']
    else:
        print(f"[ERROR] Challenge failed: {res.status_code} {res.text}")
        return None

def wait_for_game_end(game_id, token, poll_interval=2):
    headers = {"Authorization": f"Bearer {token}"}
    url = f"https://lichess.org/api/game/{game_id}"

    while True:
        res = requests.get(url, headers=headers)
        if res.status_code != 200:
            print(f"[ERROR] Could not check game state: {res.status_code}")
            break

        data = res.json()
        status = data.get("status", "").lower()
        if status in ("mate", "resign", "draw", "timeout", "outoftime", "aborted"):
            print(f"[INFO] Game ended with status: {status}")
            break

        time.sleep(poll_interval)

# === MAIN LOOP ===
while True:
    try:
        print("[INFO] Starting bots...")
        bot1 = launch_bot(bot1_env, bot1_username)
        bot2 = launch_bot(bot2_env, bot2_username)

        time.sleep(3)  # Give bots time to connect

        challenge_id = challenge_bot(bot1_token, bot2_username)
        if not challenge_id:
            print("[ERROR] Failed to challenge. Killing bots and retrying.")
            bot1.kill()
            bot2.kill()
            time.sleep(5)
            continue

        # Wait a bit for game to start and get ID
        time.sleep(5)
        game_id = challenge_id

        if not game_id:
            print("[ERROR] Game ID not found. Killing bots and retrying.")
            bot1.kill()
            bot2.kill()
            time.sleep(5)
            continue

        wait_for_game_end(game_id, bot1_token)

        print("[INFO] Game complete. Restarting bots.")
        bot1.kill()
        bot2.kill()
        time.sleep(3)
    except(KeyboardInterrupt):
        bot1.kill()
        bot2.kill()
        print("\nKeyboard Interupt Detected. Stopping bots.")
        quit()