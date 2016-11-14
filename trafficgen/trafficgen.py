"""
Example of a simple timing attack. 
"""
import sys, time, random, socket, os, struct
import threading
import binascii
import dpkt

from test_stream import *

def main():
	"""
	Runs a simple timing attack. 
	"""

	testStreamRate = 1000 # send packets at this rate during the test stream. 
	# to notice an effect in mininet, you need ~10000 packets per second.
	# to notice an effect in the pica 8, you only need ~100 pps.

	print("preparing attack")
	attackerInterface = "eth7"
	prepareSender(interface=attackerInterface)
	print ("\tgenerating packets for attack.")
	generateFixedPackets(duration=2, rate=testStreamRate)
	print ("done generating packets.")
	
	# start the timing probes.
	workloadThread = threading.Thread(target = sendWorkload)
	workloadThread.start()
	# join and finish the Workload.
	workloadThread.join()


if __name__ == '__main__':
	main()
