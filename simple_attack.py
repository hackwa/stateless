"""
Example of a simple timing attack. 
"""
import sys, time, random, socket, os, struct
import threading
import binascii
import dpkt
import pickle

from test_stream import *
from timing_probes import *


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
	print ("done geerating packets.")
	# generateRandomPackets(duration=2, rate=testStreamRate)
	
	# start the timing probes.
	probeResults = RetObj()
	probeThread = threading.Thread(target = runProbeThreads, args = (probeResults, attackerInterface, 10, 10, None))
	probeThread.start()
	time.sleep(5)

	stime, ftime = sendWorkload()
	print ("workload started at %s and lasted %s seconds"%(stime, (ftime - stime)))

	# join and finish the timing probes.
	probeThread.join()

	# get the average RTT during the workload, and normally when there's no workload.
	rrtsBaseline = []
	rttsDuringWorkload = []
	storage = {}
	print ("probe send time, RTT")
	for k in probeResults.probeRTTs.keys():
		if (probeResults.probeSentTimes[k]<stime):
		#	print ("%s, %s (DURING BASELINE)"%(probeResults.probeSentTimes[k], probeResults.probeRTTs[k]))
			if probeResults.probeRTTs[k] is not None:
				rrtsBaseline.append(probeResults.probeRTTs[k])
		elif(probeResults.probeSentTimes[k]>stime) and (probeResults.probeSentTimes[k] < ftime):
		#	print ("%s, %s (DURING WORKLOAD)"%(probeResults.probeSentTimes[k], probeResults.probeRTTs[k]))
			if probeResults.probeRTTs[k] is not None:
				rttsDuringWorkload.append(probeResults.probeRTTs[k])
	storage["probeRTTs"] = probeResults.probeRTTs.values()
	storage["probeSentTimes"] = probeResults.probeSentTimes.values()
	storage["stime"] = stime
	storage["ftime"] = ftime
	print(storage)
	output = open('sample.pkl','wb')
	pickle.dump(storage,output)
	output.close()
	print ("average RTT normally: %s"%(sum(rrtsBaseline)/len(rrtsBaseline)))
	print ("average RTT workload: %s"%(sum(rttsDuringWorkload)/len(rttsDuringWorkload)))



if __name__ == '__main__':
	main()
