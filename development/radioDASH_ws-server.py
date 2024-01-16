#!/usr/bin/python3
#
#   radioDASH WebSocket Server
#
#       USAGE: radioDASH-ws-server.py 
#
#       Ⓒ2020 Joe Cupano, NE2Z
#
#       RELEASE: 20201228-2100
#       LICENSE: GPLv2
#
#


#
# IMPORT LIBRARIES
#

import argparse 
import binascii
import sys
import hashlib
import uuid
import os
import time
import json
import ssl
import multiprocessing
import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.gen
from tornado.options import define, options


#
# VARIABLES
#

define("port", default=8000, help="run on the given port", type=int)
clients = []                                                             # Client Connection Tracking for Tornado
 
radioDASHRXLOCK = False                                                  # True = RX is running
radioDASHTXLOCK = False                                                  # True = TX is running
radioDASHfpath = ""                                                      # Path to files. Change when Pi
radioDASHcfg = "cfg/SIGnora.json"                     # Config file is in JSON format
radioDASHmsgs = 'msgs/radioDASH.msgs'                                    # radio writes msgs received here   
radioDASHpwf = "cfg/radioDASH.pwf"                      # Password file  user:hashedpasswd
stored_password = ""                                                     # hashedpassword stored in Password file


#
# CLASSES
#

class DASHini:
    def __init__(self):
        with open(radioDASHcfg) as configFileJson:
            jsonConfig = json.load(configFileJson)
        self.radio = jsonConfig["CURRENT"]["radio"]
        self.oled = jsonConfig["CURRENT"]["oled"]
        self.node_address = jsonConfig["CURRENT"]["node_address"]
        self.frequency = jsonConfig["CURRENT"]["frequency"]
        self.txpwr = jsonConfig["CURRENT"]["txpwr"]
        self.mycall = jsonConfig["CURRENT"]["mycall"]
        self.myssid = jsonConfig["CURRENT"]["myssid"]
        self.mybeacon = jsonConfig["CURRENT"]["mybeacon"]
        self.dstcall = jsonConfig["CURRENT"]["dstcall"]
        self.dstssid = jsonConfig["CURRENT"]["dstssid"]

class DASHsess:
    def __init__(self):
        self.MsgFile = 'msgs/radioDASH.msgs'
        self.currMsg = ""
        self.currMsgTs = time.time()
        self.lastMsg = ""
        self.lastMsgTs = time.time()
        self.wsMsg = "NULL:NULL"                # Websocket Message Received from Client "CMD:OPR"
        self.wsCmd = "NULL"                     # Command (CMD) parsed
        self.wsOpr = "NULL"                     # Operator (OPR) aka message parsed   

class BaseHandler(tornado.web.RequestHandler):
    def get_current_user(self):
        return self.get_secure_cookie("radioDASHuser")

class MainHandler(BaseHandler):
    @tornado.web.authenticated
    def get(self):
        self.render('SIGnora_INDEX.html')

class LoginHandler(BaseHandler):
    def get(self):
        self.render('static/radioDASH_LOGIN.html')

    def post(self):
        fusername = self.get_argument("fusername")
        fpassword = self.get_argument("fpassword")
        if find_user(fusername) == "":
            self.redirect("/login")
        stored_password = find_password(fusername)
        verdict = verify_password(stored_password, fpassword)
        if verdict == True:
            self.set_secure_cookie("radioDASHuser", str(uuid.uuid4()), secure=True, expires_days=1)
            self.redirect("/")

class WebSocketHandler(tornado.websocket.WebSocketHandler):
    def open(self):
        print ('WS: new client connection')
        clients.append(self)
        self.write_message("WS: client connected")
 
    def on_message(self, message):
        print ('WS: message from client: %s' % message)
        self.write_message('WS: ACK:')
        # Message received on the handler
        messageParse = message.split (':')
        radioDASHcmd = messageParse[0]
        radioDASHopr = messageParse[1]
        if radioDASHcmd =="RX":
            self.write_message(message)
            self.timestamp = time.time()
        else:
            print("WS: ACK:{}".format(message))
            self.write_message("ACK:{}".format(message))
            self.timestamp = time.time()
            if len(message) > 0:
               if radioDASHcmd == "TUNER":
                    if radioDASHopr == 'RESET':
                        pass
                    elif radioDASHopr == 'NUMBER':
                        pass
               if radioDASHcmd == "RADIO":
                    if radioDASHopr == 'OFF':
                        pass
                    elif radioDASHopr == 'ON':
                        pass
               if radioDASHcmd == "TXPWR":
                    if radioDASHopr == 'LOW':
                        DASHini.txpwr = 5
                    elif radioDASHopr == 'MEDIUM':
                        DASHini.txpwr = 10
                    elif radioDASHopr == 'HIGH':
                        DASHini.txpwr = 20
               if radioDASHcmd == "MODE":
                    if radioDASHopr == 'LORA':
                        pass
                    elif radioDASHopr == 'FSK':
                        pass
                    elif radioDASHopr == 'AFSK':
                        pass
                    elif radioDASHopr == 'AX25':
                        pass
                    elif radioDASHopr == 'CW':
                        pass
                    elif radioDASHopr == 'MODE':
                        pass
               if radioDASHcmd == "BEACON":
                    if radioDASHopr == 'OFF':
                        pass
                    elif radioDASHopr == 'ON':
                        pass
               if radioDASHcmd == "LOG":
                    if radioDASHopr == 'OFF':
                        pass
                    elif radioDASHopr == 'ON':
                        pass
               if radioDASHcmd == "TX":
                    if radioDASHopr == 'OFF':
                        pass
                    elif radioDASHopr == 'ON':
                        pass
    
    def on_close(self):
        print ('WS: client connection closed')
        clients.remove(self)
 

#
# GLOBAL FUNCTIONS
#

def verify_password(stored_password, provided_password):
    """Verify a stored password against one provided by username"""
    salt = stored_password[:64]
    stored_password = stored_password[64:]
    pwdhash = hashlib.pbkdf2_hmac('sha512', provided_password.encode('utf-8'), salt.encode('ascii'), 100000)
    pwdhash = binascii.hexlify(pwdhash).decode('ascii')
    return pwdhash == stored_password

def find_user(user):
    userfound=""
    f = open(radioDASHpwf, "r")
    flines = f.readlines()
    for fl in flines:
        fluser = fl.split(":")
        if user == fluser[0]:
            userfound = fluser[0]
    f.close()
    return (userfound)

def find_password(user):
    userpassword = ""
    f = open(radioDASHpwf, "r")
    flines = f.readlines()
    for fl in flines:
        fluser = fl.split(":")
        if user == fluser[0]:
            userpassword = (fluser[1]).rstrip()
    f.close()
    return (userpassword)


#
# MAIN
#
 
def main():
 
    tornado.options.parse_command_line()

    settings = {
        "cookie_secret":"gWsdN18jkIWNmksfh2poINsJxZZ83Vo=",
        "login_url": "/login",
    }

    app = tornado.web.Application(
        handlers=[
            ('/wss', WebSocketHandler),
            ('/', MainHandler),
            ('/login', LoginHandler),
            ('/css/(.*)', tornado.web.StaticFileHandler, {'path': 'css/'}),
            ('/js/(.*)', tornado.web.StaticFileHandler, {'path': 'js/'}),
            ('/cfg/(.*)', tornado.web.StaticFileHandler, {'path': 'cfg/'}),
            ('/msgs/(.*)', tornado.web.StaticFileHandler, {'path': 'msgs/'}),
            ('/(.*)', tornado.web.StaticFileHandler, {'path': 'static/'})
        ], **settings
    )
    httpServer = tornado.httpserver.HTTPServer(app,
        ssl_options = {
            "certfile": os.path.join("cfg/radioDASH.crt"),
            "keyfile": os.path.join("cfg/radioDASH.key"),
        }
    )

    httpServer.listen(options.port)
    print (" ")
    print ("radioDASH started: listening on port:", options.port)
    print (" ")
 
    def checkHasRX():   
        DASHsess.currMsgTs = os.path.getmtime(radioDASHmsgs)
        if DASHsess.currMsgTs != DASHsess.lastMsgTs:
            DASHsess.lastMsgTs = os.path.getmtime(radioDASHmsgs)
            print ('MSGS update:')
            f = open(DASHsess.MsgFile, 'r')
            currMsgJson = f.read()
            f.close()
            DASHsess.lastMsg = currMsgJson
            for c in clients:
                c.write_message("RX:" + currMsgJson)
            print ('MSGS update: sent')
    scheduler_interval = 111
    scheduler = tornado.ioloop.PeriodicCallback(checkHasRX, scheduler_interval, jitter=0.1)
    scheduler.start()
    tornado.ioloop.IOLoop.current().start()


if __name__ == "__main__":
#    DASHit = DASHini()
    DASHsess = DASHsess()
    main()
