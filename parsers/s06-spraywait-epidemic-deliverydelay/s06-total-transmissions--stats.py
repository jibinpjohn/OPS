#!/usr/bin/python
# Script to extract and process all data related entries from the
# OMNeT simulator log to compute the following statistics.
#
# - delivery ratio of data - with the Promote App
#
# Author: Asanga Udugama (adu@comnets.uni-bremen.de)
# Date: 05-dec-2016

import sys, getopt, re, tempfile

inputfile = 0
tempfile1 = 0
outputfile1 = 0
outputfile2 = 0

data_generated = 0
data_received = 0
accumilated_recvd_msgtr = 0
accumilated_notrcvd_msgtr=0

data_items = []

max_sim_time = 0.0

class Data_Item:
    def __init__(self, data_name, transmission):
        self.data_name = data_name
        self.transmission= transmission
        self.received_count = 0

def parse_param_n_open_files(argv):
    global inputfile
    global tempfile1
    global outputfile1
    global outputfile2
    global max_sim_time

    try:
        opts, args = getopt.getopt(argv,"hi:m:",["ifile=", "maxtime="])
    except getopt.GetoptError:
        print 's03-delivery-ratio.py -i <inputfile> -m <max sim time>'
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            print 's03-delivery-ratio.py -i <inputfile> -m <max sim time>'
            sys.exit()
        elif opt in ("-i", "--ifile"):
            inputfilename = arg
        elif opt in ("-m", "--maxtime"):
            max_sim_time = float(arg)

    newfilename = re.sub(':', '_', inputfilename)
    newfilename = re.sub('-', '_', newfilename)

    inputfile = open(inputfilename, "r")
    tempfile1 = tempfile.TemporaryFile(dir='.')
    outputfile1 = open(re.sub('.txt', '_dr_01.txt', newfilename), "w+")
    outputfile2 = open(re.sub('.txt', '_dr_02.txt', newfilename), "w+")

    print "Input File:                   ", inputfile.name
    print "Temporary File:               ", tempfile1.name
    print "Delivery Ratio:               ", outputfile1.name

def extract_from_log():
    global inputfile,outputfile1
    global tempfile1
    global max_sim_time
    #outputfile1.write("# data generated :: data received  :: average data delay \n")
    tempfile1.write("# all required tags from log file\n")

    for line in inputfile:

        if "INFO" in line and (("LO" in line and ("DM" in line)) or\
            "DESTINATION" in line):

            words = line.split(">!<")
            sim_time = float(words[1].strip())
            #print "simulation time is",sim_time,'Max simu time is :',max_sim_time
            if max_sim_time == 0.0 or (max_sim_time > 0.0 and sim_time <= max_sim_time):
                tempfile1.write(line)
		outputfile1.write(line)

def accumulate_and_show_delivery_data():
    global tempfile1
    global outputfile1
    global outputfile2
    global data_items
    global data_generated
    global data_received
    global accumilated_recvd_msgtr
    global accumilated_notrcvd_msgtr

    outputfile1.write("# data generated :: data received  :: average data delay \n")



    tempfile1.seek(0)

    for line in tempfile1:
        if line.strip().startswith("#"):
            continue

        if "LO" in line and ("DM" in line):
            words = line.split(">!<")
            found = False
            print words[9].strip(),":",words[10].strip()
            for data_item in data_items:
                if data_item.data_name == words[9].strip():

		    if data_item.received_count == 0:
			print 'data message',data_item.data_name,'already existing and transmission is:',data_item.transmission
		    	data_item.transmission=data_item.transmission+int(words[10].strip())
                    	print 'Now the the no. of transmission:',data_item.transmission
                    else:
			print 'The data message is already RECIEVED so don\'t have to add ',data_item.transmission

                    found = True
                    break
            if not found:
                data_item = Data_Item(words[9].strip(), int(words[10].strip()))
		print 'data message NOT FOUND:',words[9].strip()


                data_items.append(data_item)

                data_generated += 1
		print "The generated data name:",words[9].strip(),"transmission",data_items[data_generated-1].transmission
		##print data_items[data_generated-1].received_count
            #else:
            #    print 'More than once generated data ' + words[8].strip()
                #sys.exit(2)


        elif "DESTINATION" in line:
            words = line.split(">!<")
            found = False

            for data_item in data_items:
                if data_item.data_name == words[6].strip():
                   found = True
		   #print found
                   break
            if found:
                #data_item.received_count += 1
                if data_item.received_count == 0:
		    print 'The message with message ID:',words[6].strip(),'reached at destination'
                    data_received += 1
		    data_item.received_count += 1

		else:
		    data_item.received_count += 1
		    print 'message ID:',words[6].strip()
		    print 'the copy recieved',data_item.received_count
            else:
                print 'Non generated data ' + words[6].strip()
                sys.exit(2)
        else:
            print "Unknown line - " + line

    outputfile2.write( "The message ID:                         :The no. of transmission          :Message Reached at destination"+"\n")
    for data_item in data_items:
	if data_item.received_count == 0:
        	accumilated_notrcvd_msgtr=accumilated_notrcvd_msgtr+int(data_item.transmission)
		outputfile2.write( str(data_item.data_name)+ "               :"+str(data_item.transmission)+"                                :NO" + "\n")
	else:
        	accumilated_recvd_msgtr=accumilated_recvd_msgtr+int(data_item.transmission)
		outputfile2.write( str(data_item.data_name)+ "               :"+str(data_item.transmission)+"                                :YES" + "\n")
    outputfile2.write("Total no. of transmission of all the received messages                                        :: " + "Total no. of transmission for messages yet to reach destination                                 :: Total transmission"+ "\n")
    outputfile2.write( str(accumilated_recvd_msgtr)+ "                                        :: " + str(accumilated_notrcvd_msgtr) + "                                 :: " + str(accumilated_recvd_msgtr+accumilated_notrcvd_msgtr) + "\n")
    outputfile2.write("The averege #transmission of all the received messages                                       :: " + "The averege #transmission for messages yet to reach destination                                 :: The Total Average" +"\n")
    outputfile2.write( str(accumilated_recvd_msgtr/float(data_received))+ "                                        :: " + str(accumilated_notrcvd_msgtr/float(data_generated-data_received)) + "                                 :: " + str((accumilated_recvd_msgtr+accumilated_notrcvd_msgtr)/float(data_generated)) + "\n")

    print data_received, ":                    " ,(data_generated-data_received),":                    ",float(data_generated)
    #outputfile2.write(str(data_generated) + " :: "+ "\n")

def close_files():
    global inputfile
    global tempfile1
    global outputfile1
    global outputfile2

    inputfile.close()
    tempfile1.close()
    outputfile1.close()
    outputfile2.close()

def main(argv):
    parse_param_n_open_files(argv)
    extract_from_log()
    accumulate_and_show_delivery_data()
    close_files()

if __name__ == "__main__":
    main(sys.argv[1:])
