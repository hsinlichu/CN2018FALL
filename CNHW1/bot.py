#!/usr/bin/python
#coding:big5

import sys 
import socket
from random import randint
import select
import urllib.request
from bs4 import BeautifulSoup as BS
from urllib.parse import quote

IRC = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
server = '127.0.0.1'
port=6667
channel = "#CN_DEMO" # Channel
botnick = "bot_b05505004" # Your bots nick

constellation = {"Capricorn":"歡歡喜喜的才會有好運降臨。",
                 "Aquarius":"嘗試也能有所收穫，切忌一昧守候。",
                 "Pisces":"大膽追求激情。",
                 "Aries":"今天是展現口才的一天。",
                 "Taurus":"心態決定事物取向。",
                 "Gemini":"清楚瞭解自己想要的後再付諸行動。",
                 "Cancer":"愛情要多增加點溫度。",
                 "Leo":"對待他人還是寬容些的好。",
                 "Virgo":"處事時要慎重為之。",
                 "Libra":"運勢一般，多了幾分和氣，少了幾分刻板。",
                 "Scorpio":"知足常樂、感恩常樂。",
                 "Sagittarius":"運勢很好，注意坦率直接。"}

IRC.connect((server,port))
IRC.send(bytes("NICK "+ botnick + "\n", "UTF-8"))
IRC.send(bytes("USER "+ botnick + "\n", "UTF-8"))

def joinchan(chan): # join channel(s).
  IRC.send(bytes("JOIN "+ chan +"\n", "UTF-8")) 
  ircmsg = ""
  while ircmsg.find("End of /NAMES list.") == -1:  
    ircmsg = IRC.recv(2048).decode("UTF-8")
    ircmsg = ircmsg.strip('\n\r')
    #print(ircmsg)

def ping(): # respond to server Pings.
    #print("send: pong")
    IRC.send(bytes("PONG\n", "UTF-8"))

def sendmsg(msg, target=channel): # sends messages to the target.
  tmp = "PRIVMSG "+ target +" :"+ msg +"\n"
  #print(tmp)
  IRC.send(bytes(tmp, "UTF-8"))

def guess(nick,message,ans):
  try: 
    x = int(message)
  except ValueError:
    if message in constellation:
      sendmsg("猜完數字才給你問星座運勢",nick)
    sendmsg("請輸入數字 =.=",nick)
    return 0
  x = int(message)
  if 1 <= x <= 10:
    if x == ans:
      sendmsg("恭喜猜中! 答案為%d"%(ans),nick)
      return 1
    elif x > ans:
      sendmsg("再小一點點",nick)
    else:
      sendmsg("再大一點點",nick)
  else:
    sendmsg("請輸入介於1~10的數字!!",nick)
  return 0

def chat(nick,message):
  if message.find("!bye") != -1:
	  print("\r"+nick+"> "+message)
	  tmp = "\r======= "+nick+" 已離開聊天室 ======="
	  print(tmp)
	  return 1
  else:
    print("\r"+nick+"> "+message)
    return 0

def song(ircmsg,client_nick):
  query = quote(ircmsg.split(" ")[-1])
  url = 'https://www.youtube.com/results?search_query=' + query
          
  #print(url)
  with urllib.request.urlopen(url) as page:
    html = page.read()
    soup = BS(html,'html.parser')
    a = soup.find_all('a')
    link = None
    for tag in a:
      link  = tag.get('href')
      if link.find('/watch?v=') != -1:
        #print(tag)
        break
    sendmsg("https://www.youtube.com"+link,client_nick)
    return None
  
def main():
  joinchan(channel)
  sendmsg("I'm b05505004")
  chatting = []
  guessing = {}
  IRC.setblocking(False)
  while True:
    sys.stdout.flush()
    if len(chatting) == 0:
      #print("Waiting for command...")
      readable = select.select([IRC,],[],[], None )[0]
    else:
      #print("Waiting for command or reply to ",chatting)
      print("\r> ",end='')
      sys.stdout.flush()
      readable = select.select([sys.stdin,IRC],[],[],None)[0]
    if IRC in readable:
      ircmsg = IRC.recv(2048).decode("UTF-8")
      ircmsg = ircmsg.strip('\n\r')
      #print("Receive:",ircmsg)
      if ircmsg.find("PING") != -1:
        ping()
        continue
      client_nick = ircmsg.split("!",1)[0][1:]
      message = ircmsg.split(" :")[-1]

      if client_nick in guessing:
        if guess(client_nick,message,guessing[client_nick]):
          del guessing[client_nick]
        continue
      if client_nick in chatting:
        if chat(client_nick,message):
          chatting.remove(client_nick)
        continue

      if ircmsg.find("PRIVMSG") != -1:
        if ircmsg.find("!guess") != -1:
          guessing[client_nick] = randint(1,10)
          sendmsg("請輸入任一1~10的數字：",client_nick)
        elif ircmsg.find("!song") != -1:
          song(ircmsg,client_nick)
        elif ircmsg.find("!chat") != -1:
          tmp = "\r======= "+client_nick+"想和你聯絡 ======="
          print(tmp)
          chatting.append(client_nick)
        elif message in constellation:
          sendmsg("今日運勢："+constellation[message],client_nick)
        else:
          print("\r"+client_nick+"> "+message)
          tmp = "並無這項指令"
          sendmsg(tmp,client_nick)
      else:
        print("\r"+client_nick+"> "+message)
        #print("can't recognize")
        #print(ircmsg)
                  
    elif sys.stdin in readable:
      tmp = sys.stdin.readline().strip('\n\r')
      tmp2 = tmp.split(' ',1)
      if tmp2[0] not in chatting:
        client_nick,response = chatting[0],tmp
      else:
        client_nick,response = tmp2
      #print(client_nick,response)
      sendmsg(response,client_nick)


if __name__ == '__main__':
    main()
