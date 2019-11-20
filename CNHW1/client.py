#!/usr/bin/python
#coding:big5

import socket
import sys
import select


IRC = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
server = '127.0.0.1'
port=6667
channel = "#CN_DEMO" # Channel
botnick = "bot_b05505004" # Your bots nick
nick = sys.argv[1]
exitcode = "bye " + botnick

IRC.connect((server,port))
IRC.send(bytes("USER "+ nick + "\n", "UTF-8"))
IRC.send(bytes("NICK "+ nick + "\n", "UTF-8"))


def joinchan(chan): # join channel(s).
  IRC.send(bytes("JOIN "+ chan +"\n", "UTF-8")) 
  ircmsg = ""
  while ircmsg.find("End of /NAMES list.") == -1:  
    ircmsg = IRC.recv(2048).decode("UTF-8")
    ircmsg = ircmsg.strip('\n\r')
    #print(ircmsg)

def ping(): # respond to server Pings.
    #print("pong")
    IRC.send(bytes("PONG\n", "UTF-8"))

def sendmsg(msg, target=channel): # sends messages to the target.
  target = "#CN_DEMO"
  tmp = "PRIVMSG "+ target +" :"+ msg +"\n"
  #print(tmp)
  IRC.send(bytes(tmp, "UTF-8"))

def main():
  chatting = False
  joinchan(channel)
  #IRC.send(bytes("INFO","UTF-8"))
  IRC.setblocking(False)
  while True:
    sys.stdout.flush()
    print("\r> ",end='')
    sys.stdout.flush()
    readable = select.select([sys.stdin,IRC],[],[],None)[0]
    if IRC in readable:
      ircmsg = IRC.recv(2048).decode("UTF-8")
      ircmsg = ircmsg.strip('\n\r')
      if ircmsg.find("PING") != -1:
        ping()
      elif ircmsg.find("!"):
        client_nick = ircmsg.split("!",1)[0][1:]
        message = ircmsg.split(" :")[-1]
        print("\r"+client_nick+"> "+message)
      else:
        print("\r"+ircmsg)
    elif sys.stdin in readable:
      s = sys.stdin.readline()
      sendmsg(s,botnick)
      if s.find("!chat") != -1:
        chatting = True
        tmp = "\r======= 正在聯絡"+botnick+" ======="
        print(tmp)
      if chatting and s.find("!bye") != -1:
        tmp = "\r======= 您已關閉聊天室 ======="
        print(tmp)
    
    


if __name__ == '__main__':
    main()
