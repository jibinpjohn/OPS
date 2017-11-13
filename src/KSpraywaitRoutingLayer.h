//
// The model implementation for the Epidemic Routing layer
//
// @author : Jibin John (adu@comnets.uni-bremen.de)
// @date   : 02-may-2017
//
//

#ifndef KSPRAYWAITROUTINGLAYER_H_
#define KSPRAYWAITROUTINGLAYER_H_

#define TRUE                            1
#define FALSE                           0

#include <omnetpp.h>
#include <cstdlib>
#include <sstream>
#include <string>

#include "KOPSMsg_m.h"
#include "KInternalMsg_m.h"

#if OMNETPP_VERSION >= 0x500
using namespace omnetpp;
#endif

using namespace std;

class KSpraywaitRoutingLayer: public cSimpleModule
{
    protected:
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual int numInitStages() const;
        virtual void finish();

    private:
        string ownMACAddress;
        int nextAppID;
        int maximumCacheSize;
        double antiEntropyInterval;
        int maximumHopCount;
        double maximumRandomBackoffDuration;
        int logging;
	int newcopies;
	int L;

        int currentCacheSize;

        struct AppInfo {
            int appID;
            string appName;
            string prefixName;
        };

        struct CacheEntry {
            string messageID;
            int hopCount;
	    int copies;
            string dataName;
            int realPayloadSize;
            string dummyPayloadContent;

            double validUntilTime;

            int realPacketSize;

            bool destinationOriented;
            string originatorNodeName;
            string finalDestinationNodeName;

            int goodnessValue;


            double createdTime;
            double updatedTime;
            double lastAccessedTime;

        };

        struct SyncedNeighbour {
            string nodeMACAddress;
            double syncCoolOffEndTime;

            bool randomBackoffStarted;
            double randomBackoffEndTime;

            bool neighbourSyncing;
            double neighbourSyncEndTime;

            bool nodeConsidered;

        };

        list<AppInfo*> registeredAppList;
        list<CacheEntry*> cacheList;
        list<SyncedNeighbour*> syncedNeighbourList;
        bool syncedNeighbourListIHasChanged;

        cMessage *ageDataTimeoutEvent;

        void handleDataAgingTrigger(cMessage *msg);
        void handleAppRegistrationMsg(cMessage *msg);
        void handleDataMsgFromUpperLayer(cMessage *msg);
        void handleNeighbourListMsgFromLowerLayer(cMessage *msg);
        void handleDataMsgFromLowerLayer(cMessage *msg);
        void handleSummaryVectorMsgFromLowerLayer(cMessage *msg);
        void handleDataRequestMsgFromLowerLayer(cMessage *msg);

        SyncedNeighbour* getSyncingNeighbourInfo(string nodeMACAddress);
        void setSyncingNeighbourInfoForNextRound();
        void setSyncingNeighbourInfoForNoNeighboursOrEmptyCache();
        KSummaryVectorMsg* makeSummaryVectorMessage();

};
#define KSPRAYWAITROUTINGLAYER_SIMMODULEINFO          ">!<" << simTime() << ">!<" << getParentModule()->getFullName()
#define KSPRAYWAITROUTINGLAYER_DEBUG                  ">!<DEBUG>!<" << ownMACAddress

#define KSPRAYWAITROUTINGLAYER_MSG_ID_HASH_SIZE      4 // in bytes

#endif /* KEPIDEMICROUTINGLAYER_H_ */