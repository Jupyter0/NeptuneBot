import subprocess
import requests
import time
import os
import json

# Set these before running
BOT_A_ENV = ".env.botA"
BOT_B_ENV = ".env.botB"
BOT_A_USERNAME = "NeptuneBotA"
BOT_B_USERNAME = "NeptuneBotB"
BOT_A_TOKEN = os.getenv("BOT_A_TOKEN")
BOT_B_TOKEN = os.getenv("BOT_B_TOKEN")

def start_bot(env_file):
    return subprocess.Popen(["dotnet", "run", env_file])

def send_challenge(from_token, to_username):
    headers = {"Authorization": f"Bearer {from_token}"}
    res = requests.post(f"https://lichess.org/api/challenge/{to_username}", headers=headers)
    if res.status_code == 200:
        challenge_id = res.json()['challenge']['id']
        print(f"[INFO] Challenge sent: {challenge_id}")
        return challenge_id
    else:
        print(f"[ERROR] Failed to send challenge: {res.text}")
        return None

def wait_for_game_start(from_token, challenge_id):
    headers = {"Authorization": f"Bearer {from_token}"}
    stream_url = f"https://lichess.org/api/stream/event"
    with requests.get(stream_url, headers=headers, stream=True) as res:
        for line in res.iter_lines():
            if not line: continue
            event = json.loads(line.decode("utf-8"))
            if event.get("type") == "gameStart":
                game_id = event["game"]["id"]
                print(f"[INFO] Game started: {game_id}")
                return game_id

def wait_for_game_end(bot_token, game_id):
    headers = {"Authorization": f"Bearer {bot_token}"}
    url = f"https://lichess.org/api/bot/game/stream/{game_id}"
    with requests.get(url, headers=headers, stream=True) as res:
        for line in res.iter_lines():
            if not line: continue
            event = json.loads(line.decode("utf-8"))
            if event.get("type") == "gameFull":
                print(f"[DEBUG] Game started: {game_id}")
            elif event.get("type") == "gameState":
                if event.get("status") in ("mate", "resign", "timeout", "draw", "aborted"):
                    print(f"[INFO] Game over: {event['status']}")
                    return

def main():
    print("[INFO] Starting bots...")
    bot_a_proc = start_bot(BOT_A_ENV)
    bot_b_proc = start_bot(BOT_B_ENV)
    time.sleep(3)

    try:
        while True:
            challenge_id = send_challenge(BOT_A_TOKEN, BOT_B_USERNAME)
            if not challenge_id:
                time.sleep(5)
                continue

            game_id = wait_for_game_start(BOT_A_TOKEN, challenge_id)
            if not game_id:
                continue

            wait_for_game_end(BOT_A_TOKEN, game_id)
            print("[INFO] Restarting for next game...\n")
            time.sleep(2)
    finally:
        print("[INFO] Shutting down bots.")
        bot_a_proc.kill()
        bot_b_proc.kill()

if __name__ == "__main__":
    if not BOT_A_TOKEN or not BOT_B_TOKEN:
        print("[ERROR] Set BOT_A_TOKEN and BOT_B_TOKEN as environment variables.")
        exit(1)
    main()
