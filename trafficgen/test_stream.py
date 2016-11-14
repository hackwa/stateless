"""
Functions to send a test stream of packets into the network. 
Uses click to send accurately.
Call this after you start the timing probes. 

Usage: 
# generate the click config file for your attacker host.
prepareSender(interface="eth7")

# generate a workload PCAP file, dumpped to the default file.
generateRandomPackets(duration=10, rate=100)

# (code to start timing probes or whatever goes here)

# send the workload in the default file.
sendWorkload()

# code to analyze timing probes goes here.

"""
import sys, time, random, socket, os, struct
import threading
import binascii
import dpkt
import subprocess

default_clickFile = "sendWorkload.click"
default_pktTmpFile = 'workload.pcap'
default_interface = "eth7"

def generateFixedPackets(duration = 10, rate = 100, dumpFile = default_pktTmpFile):
	"""
	Generates UDP packet with a fixed source and destination IP and ethernet address.
	"""
	eth_src = '\x22\x22\x22\x22\x22\x11'
	eth_dst = '\x22\x22\x22\x22\x22\x22'
	ip_src = socket.inet_aton("111.111.111.111")
	ip_dst = socket.inet_aton("222.222.222.222")
	udp_sport = 11111
	udp_dport = 22222
	payload = "Hello Network" # for debugging.

	packetCt = rate * duration
	f = startPcapFile(dumpFile)
	current_time = 0.0
	for i in range(packetCt):
		udpOut = dpkt.udp.UDP()
		udpOut.data = payload
		udpOut.sport = udp_sport
		udpOut.dport = udp_dport
		udpOut.ulen = len(udpOut)
		ipOut = dpkt.ip.IP(src=ip_src, dst=ip_dst)
		ipOut.p = 0x11 # protocol.
		ipOut.data = udpOut
		ipOut.v = 4
		ipOut.len = len(ipOut)
		ethOut = dpkt.ethernet.Ethernet(src=eth_src, dst = eth_dst, \
			type = dpkt.ethernet.ETH_TYPE_IP, data = ipOut)
		ethOutStr = str(ethOut)	
		writePktToFile(ethOutStr, current_time, f)
		current_time += 1.0 / rate
	f.close()


def generateRandomPackets(duration = 10, rate = 100, dumpFile = default_pktTmpFile):
	"""
	Generates ethernet packets with random sources and addresses, 
	dumps them to the specified pcap file and sets timestamps so that there are 
	_rate_ packets per second.
	"""
	print ("generating workload %s packets / second for %s seconds. Dumping to %s"%(rate, duration, dumpFile))
	# generate a random ethernet header for each packet. 
	packetCt = rate * duration
	headers = generatePacketHeaderList(packetCt)
	# generate ethernet packets based on the headers.
	packetList = [generateEthPacket(h[0], h[1]) for h in headers]
	# dump the packets to the file. 
	if packetList != None:
		f = startPcapFile(dumpFile)
		current_time = 0.0
		for pkt in packetList:
			writePktToFile(pkt, current_time, f)
			current_time += 1.0/rate
		f.close()
		# print("%s workload packets written to temp file (rate = %s packets / sec)."%(packetCt, pps))


def generatePacketHeaderList(ct):
	"""
	generates random src / dest mac pairs.
	"""
	return [('\x00\x00\x00'+ os.urandom(3), '\x00\x00\x00'+ os.urandom(3)) for i in range(ct)]


def generateEthPacket(esrc, edst):
	eth = dpkt.ethernet.Ethernet()
	eth.src = esrc
	eth.dst = edst
	eth.type = 0x809B
	eth.data = ''
	# pad with random bytes.
	padlen = 46 - len(eth)
	eth.data = eth.data + "".join(['\x00' for i in range(padlen)])
	# gotta return an eth packet, no matter what layer its at.
	return str(eth)


def prepareSender(interface = default_interface, workloadFile = default_pktTmpFile):
	"""
	Generate the click config file that sends the workload. 
	"""
	print ("generating click file %s"%default_clickFile)

	content = """
source1 :: FromDump(%s, MMAP true, TIMING true, STOP true);
source1 -> ToDevice(%s, BURST 1);
	"""%(workloadFile, interface)
	f = open(default_clickFile, "w")
	f.write(content)
	f.close()
	return

def sendWorkload(workloadFile = default_pktTmpFile, clickConfigFile = default_clickFile):
	"""
	Send out the workload. Returns the time when the sending began and finished.
	"""
	print ("sending workload with click.")
	cmd = "sudo ./click %s"%clickConfigFile	
	stime = time.time()
	subprocess.call(cmd, stderr = subprocess.PIPE, stdout=subprocess.PIPE,shell = True)
	ftime = time.time()
	print ("click returned.")
	print("total time taken = ",ftime-stime)


# pcap helpers.
#Global header for pcap 2.4
pcap_global_header="d4c3b2a1".decode("hex") + struct.pack("H",2) + struct.pack("H",4) + struct.pack("I", 0) + struct.pack("I", 0) + struct.pack("I", 1600) + struct.pack("I", 1)
pcap_packet_header = "AA779F4790A20400".decode("hex") # then put the frame size twice in little endian ints.

def appendByteStringToFile(bytestring, f):
	f.write(bytestring)

def startPcapFile(filename):
	f = open(filename, "wb")
	f.write(pcap_global_header)
	return f

def writePktToFile(pkt, ts, f):
	"""
	Writes an ethernet packet to a file. Prepends the pcap packet header.
	"""
	pcap_len = len(pkt)
	seconds = int(ts)
	microseconds = int((ts - int(ts)) * 1000000)
	bytes = struct.pack("<i",seconds) + struct.pack("<i",microseconds) + struct.pack("<i", pcap_len) + struct.pack("<i", pcap_len) + pkt
	# bytes = pcap_packet_header + struct.pack("<i", pcap_len) + struct.pack("<i", pcap_len) + pkt
	appendByteStringToFile(bytes, f)
