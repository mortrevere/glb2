import socket
import sys
import threading
import time
COLORS = {
    "black" : (0,0,0),
    "red" : (255,0,0),
    "green" : (0,255,0),
    "blue" : (0,0,255),
    "yellow" : (255, 127, 0), #jaune
    "orange" : (255, 37, 0), #jaune
    "cyan" : (0, 255, 255), #cyan
    "purple" : (255, 0, 255), #violet
    "pink" : (255, 50, 50), #violet
    "white" : (255, 255, 255)
}

generic_cmds = {
    "dim" : "d",
    "disco" : "A",
    "full" : "f",
    "off" : "b",
    "tension" : "B",
    "twist" : "C",
    #"" : "D",
    "colrot" : "E",
}

BASE_IP = "192.168.31." # change accordingly, beLow's router is set to this subnet 
TARGETS = [131, 241, 165, 219] # change accordingly, the 4 beLow globes are set to those IP

class GlobeController():
    def __init__(self, port = 10000):
        self.stop = False
        self.PORT = port
        self.socks = []
        #self.sock.bind(("0.0.0.0", self.PORT))
        #self.sock.settimeout()
        self.peers = {}
        
        self.groups = {}
        self.current_group = ""

        self.log = []

        self.discover_peers()
        self.input = ""
        self.input_thread = threading.Thread(target=self.handle_input)
        self.input_thread.start()

        
        self.input_thread.join()


        

    def discover_peers(self):
        print("Probing LAN ...")
        
        for target in TARGETS:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            
            IP = BASE_IP + str(target)
            print(IP, end="")
            print(" ...", end="", flush=True)
    
            
            
            #self.sock.sendto(b"ping", (IP, self.PORT))

            try:
                connect = sock.connect_ex((IP, self.PORT))
                if connect == 0:
                    sock.sendall(b"70") #send a ping
                    data = sock.recv(1024)
                    data = data.decode("utf-8")
                    print(data)
                    if data.startswith("BLW"):
                        self.peers[data] = {"sock": sock, "ip" : IP}
            except socket.timeout:
                print("error: socket timeout")
                pass
        print()
        self.cmd("ls")

    
    def handle_input(self):
        while self.input != "exit":
            try:
                self.input = input("\n>")
                self.cmd(self.input)
            except KeyboardInterrupt:
                pass

    def send_to_group(self, message, group = "current"):
        if group == "current":
            group = self.current_group

        split = message.split(" ")
        bytes_out = []
        for chunk in split:
            if chunk.isdigit():
                bytes_out += [chr(int(chunk))]
            elif chunk.isalpha() and len(chunk) == 1:
                bytes_out += [chunk]
            else:
                print("WARNING : couldn't figure out how to parse " + chunk)
        
        print(bytes_out, "[" + "".join(bytes_out) + "]")
        for peer in self.groups[group]:
            peer_ip = self.peers["BLW " + peer]["ip"]
            print(peer_ip)
            out = "".join(["{0:02x}".format(ord(b)) for b in bytes_out])
            print(out)
            self.peers["BLW " + peer]["sock"].send(out.encode("utf-8"))

    def cmd(self, cmd):
        self.input = cmd.strip()
        if self.input == "ls":
            for p in self.peers.keys():
                print(p)
        if self.input == "ls groups":
            for name, peers in self.groups.items():
                print(name)
                for p in peers:
                    print("  - " + p)
        if self.input.startswith("group add"):
            if not "name" in self.input:
                print("Missing 'name <groupname>' at the end of command")
                return
            chunks = self.input.split(" ")[2:]
            name = chunks[-1]
            if not self.groups.get(name):
                self.groups[name] = [] 
            self.groups[name] += chunks[0:-2]
            print(self.groups)
            print(chunks, name)

        if self.input.startswith("group select"):
            chunks = self.input.split(" ")[2:]
            name = chunks[-1]
            if name in self.groups:
                self.current_group = name
                print("OK")
            else:
                print("Couldn't find group " + name)
                return

        if self.input == "save":
            with open("save.log", "w") as f:
                f.write("\n".join(self.log))
        elif self.input == "load":
            with open("save.log", "r") as f:
                lines = f.read().split("\n")
                for l in lines:
                    if l != "load":
                        self.cmd(l)
        elif self.input == "log clear":
            self.log = []
        elif self.input == "log":
            print("\n".join(self.log))
            

        elif self.input.startswith("save"):
            chunks = self.input.split(" ")
            name = chunks[1]   
            with open(name + ".log", "w") as f:
                f.write("\n".join(self.log))
        elif self.input.startswith("loop"):
            self.loop_start = time.time()
            chunks = self.input.split(" ")
            duration = chunks[-1]
            name = chunks[1] 
            lines = []
            with open(name + ".log", "r") as f:
                lines = f.read().split("\n")
            i = 0
            duration = int(chunks[-1])
            while time.time() - self.loop_start < duration*1000:
                #for l in lines:   
                
                l = lines[i%len(lines)]
                if l != "load":
                    #if time.time() - self.loop_start > duration*1000:
                    #    break
                    if l.startswith("pause") or l.startswith("sleep"):
                        time.sleep(int(l.split(" ")[-1]))
                    if not self.stop:
                        self.cmd(l)
                i += 1
        elif self.input.startswith("load"):
            chunks = self.input.split(" ")
            name = chunks[1]
        
            with open(name + ".log", "r") as f:
                lines = f.read().split("\n")
                #lines = _lines
                
                i = 0
                for l in lines:   
                    l = lines[i%len(lines)]

                    if l != "load":
                        #if time.time() - self.loop_start > duration*1000:
                        #    break
                        if l.startswith("pause"):
                            time.sleep(int(l.split(" ")[-1]))
                        if not self.stop:
                            self.cmd(l)
                    i += 1

        if self.input.startswith("color"):
            chunks = self.input.split(" ")[1:]
            if not len(chunks) == 1:
                print("t'as foiré frr")
                return False
            color = COLORS[chunks[-1]]
            self.send_to_group("c " + " ".join([str(c) for c in color]))


        #if self.input == "off":
        #    self.send_to_group("b")

        for generic_cmd, alias in generic_cmds.items():
            if self.input.startswith(generic_cmd):
                chunks = self.input.split(" ")[1:]
                self.send_to_group(alias + " " + " ".join(chunks))

        if self.input.startswith("rb"):
            chunks = self.input.split(" ")[1:]
            self.send_to_group("r " + " ".join(chunks))

        if self.input.startswith("strobe"):
            color = COLORS["red"]
            for i in range(1000):
                self.send_to_group("c " + " ".join([str(c) for c in color]))
                time.sleep(0.1)
                self.send_to_group("b")
                time.sleep(0.1)

        if self.input not in ("log", "log clear"):
            self.log += [self.input]
        self.stop = False

try:
    a = GlobeController()
except Exception as e:
    a.stop = True
