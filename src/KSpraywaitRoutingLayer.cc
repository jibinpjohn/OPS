//
// The model implementation for the Epidemic Routing layer
//
// @author : Jibin John (adu@comnets.uni-bremen.de)
// @date   : 02-may-2017
//
//

#include "KSpraywaitRoutingLayer.h"

Define_Module(KSpraywaitRoutingLayer);

void KSpraywaitRoutingLayer::initialize(int stage)
{



    if (stage == 0) {
        // get parameters
        ownMACAddress = par("ownMACAddress").stringValue();
        nextAppID = 1;
        maximumCacheSize = par("maximumCacheSize");
        currentCacheSize = 0;
        antiEntropyInterval = par("antiEntropyInterval");
        maximumHopCount = par("maximumHopCount");
        maximumRandomBackoffDuration = par("maximumRandomBackoffDuration");
        logging = par("logging");

        syncedNeighbourListIHasChanged = TRUE;

	L=par("noDuplicate");
	EV_FATAL << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << "TheNOOFCOPIES:"<<L<<" \n";

    } else if (stage == 1) {


    } else if (stage == 2) {

        // setup the trigger to age data
        ageDataTimeoutEvent = new cMessage("Age Data Timeout Event");
        ageDataTimeoutEvent->setKind(108);
        scheduleAt(simTime() + 1.0, ageDataTimeoutEvent);

    } else {
        EV_FATAL << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << "Something is radically wrong in initialisation \n";
    }


}

int KSpraywaitRoutingLayer::numInitStages() const
{
    return 3;
}

void KSpraywaitRoutingLayer::handleMessage(cMessage *msg)
{
    cGate *gate;
    char gateName[64];
    KNeighbourListMsg *neighListMsg;

    // self messages
    if (msg->isSelfMessage()) {

        // age data trigger fired
        if (msg->getKind() == 108) {

            handleDataAgingTrigger(msg);

        } else {
            EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << "Received unexpected self message" << "\n";
            delete msg;
        }

    // messages from other layers
    } else {

       // get message arrival gate name
        gate = msg->getArrivalGate();
        strcpy(gateName, gate->getName());

        // app registration message arrived from the upper layer (app layer)
        if (strstr(gateName, "upperLayerIn") != NULL && dynamic_cast<KRegisterAppMsg*>(msg) != NULL) {

            handleAppRegistrationMsg(msg);

        // data message arrived from the upper layer (app layer)
        } else if (strstr(gateName, "upperLayerIn") != NULL && dynamic_cast<KDataMsg*>(msg) != NULL) {

            handleDataMsgFromUpperLayer(msg);

        // feedback message arrived from the upper layer (app layer)
        } else if (strstr(gateName, "upperLayerIn") != NULL && dynamic_cast<KFeedbackMsg*>(msg) != NULL) {

            // with Epidemic Routing, no feedback is considered

            delete msg;

        // neighbour list message arrived from the lower layer (link layer)
        } else if (strstr(gateName, "lowerLayerIn") != NULL && dynamic_cast<KNeighbourListMsg*>(msg) != NULL) {

            handleNeighbourListMsgFromLowerLayer(msg);

        // data message arrived from the lower layer (link layer)
        } else if (strstr(gateName, "lowerLayerIn") != NULL && dynamic_cast<KDataMsg*>(msg) != NULL) {

            handleDataMsgFromLowerLayer(msg);

        // feedback message arrived from the lower layer (link layer)
        } else if (strstr(gateName, "lowerLayerIn") != NULL && dynamic_cast<KFeedbackMsg*>(msg) != NULL) {

            // with Epidemic Routing, no feedback is considered
            delete msg;

        // summary vector message arrived from the lower layer (link layer)
        } else if (strstr(gateName, "lowerLayerIn") != NULL && dynamic_cast<KSummaryVectorMsg*>(msg) != NULL) {

            handleSummaryVectorMsgFromLowerLayer(msg);

        // data request message arrived from the lower layer (link layer)
        } else if (strstr(gateName, "lowerLayerIn") != NULL && dynamic_cast<KDataRequestMsg*>(msg) != NULL) {

            handleDataRequestMsgFromLowerLayer(msg);

        // received some unexpected packet
        } else {

            EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << "Received unexpected packet" << "\n";
            delete msg;
        }
    }
}

void KSpraywaitRoutingLayer::handleDataAgingTrigger(cMessage *msg)
{

    // remove expired data items
    int originalSize = cacheList.size();
    int expiredFound = TRUE;
    while (expiredFound) {
        expiredFound = FALSE;

        CacheEntry *cacheEntry;
        list<CacheEntry*>::iterator iteratorCache;
        iteratorCache = cacheList.begin();
        while (iteratorCache != cacheList.end()) {
            cacheEntry = *iteratorCache;
            if (cacheEntry->validUntilTime < simTime().dbl()) {
                expiredFound = TRUE;
                break;
            }
            iteratorCache++;
        }
        if (expiredFound) {

          //jibin  EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << KSPRAYWAITROUTINGLAYER_DEBUG << " :: Removing Expired Data Entry :: "
            //jibin     << cacheEntry->dataName << " :: Valid Until :: " << cacheEntry->validUntilTime << "\n";


            currentCacheSize -= cacheEntry->realPacketSize;
            cacheList.remove(cacheEntry);
            delete cacheEntry;

        }
    }

/*    EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << KSPRAYWAITROUTINGLAYER_DEBUG << " :: New Cache Size :: "
     << cacheList.size() << " :: Removed Count :: " << (originalSize - cacheList.size()) << "\n";*/

    // setup next age data trigger
    scheduleAt(simTime() + 1.0, msg);

}

void KSpraywaitRoutingLayer::handleAppRegistrationMsg(cMessage *msg)
{
    KRegisterAppMsg *regAppMsg = dynamic_cast<KRegisterAppMsg*>(msg);
    AppInfo *appInfo = NULL;
    int found = FALSE;
    list<AppInfo*>::iterator iteratorRegisteredApps = registeredAppList.begin();


    while (iteratorRegisteredApps != registeredAppList.end()) {
        appInfo = *iteratorRegisteredApps;
        if (appInfo->appName == regAppMsg->getAppName()) {
            found = TRUE;
            break;
        }
        iteratorRegisteredApps++;
    }

    if (!found) {
        appInfo = new AppInfo;
        appInfo->appID = nextAppID++;
        appInfo->appName = regAppMsg->getAppName();
        registeredAppList.push_back(appInfo);

    }
    appInfo->prefixName = regAppMsg->getPrefixName();
    delete msg;



}

void KSpraywaitRoutingLayer::handleDataMsgFromUpperLayer(cMessage *msg)
{
    KDataMsg *omnetDataMsg = dynamic_cast<KDataMsg*>(msg);



    if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<UI>!<DM>!<" << omnetDataMsg->getSourceAddress() << ">!<"
        << omnetDataMsg->getDestinationAddress() << ">!<" << omnetDataMsg->getDataName() << ">!<" << omnetDataMsg->getGoodnessValue() << ">!<"
        << omnetDataMsg->getByteLength() << "\n";}

		//jibin if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<UI>!<DM>!<" << omnetDataMsg->getSourceAddress() << ">The DESTINATION address is<"
      //jibin  << omnetDataMsg->getFinalDestinationNodeName() << ">!dest<" << omnetDataMsg->getDataName() << ">!<" << omnetDataMsg->getGoodnessValue() << ">!<"
       //jibin << omnetDataMsg->getByteLength() << "\n";}

    CacheEntry *cacheEntry;
    list<CacheEntry*>::iterator iteratorCache;
    int found = FALSE;
    iteratorCache = cacheList.begin();
    while (iteratorCache != cacheList.end()) {
        cacheEntry = *iteratorCache;
        if (cacheEntry->dataName == omnetDataMsg->getDataName()) {
            found = TRUE;
            break;
        }

        iteratorCache++;
    }

    if (!found) {

        // apply caching policy if limited cache and cache is full
        if (maximumCacheSize != 0
                && (currentCacheSize + omnetDataMsg->getRealPacketSize()) > maximumCacheSize
                && cacheList.size() > 0) {
            iteratorCache = cacheList.begin();
            advance(iteratorCache, 0);
            CacheEntry *removingCacheEntry = *iteratorCache;
            iteratorCache = cacheList.begin();
            while (iteratorCache != cacheList.end()) {
                cacheEntry = *iteratorCache;
                if (cacheEntry->validUntilTime < removingCacheEntry->validUntilTime) {
                    removingCacheEntry = cacheEntry;
                }
                iteratorCache++;
            }
            currentCacheSize -= removingCacheEntry->realPacketSize;

            cacheList.remove(removingCacheEntry);
            delete removingCacheEntry;

        }

        cacheEntry = new CacheEntry;

        cacheEntry->messageID = omnetDataMsg->getDataName();
        cacheEntry->hopCount = 0;
		cacheEntry->copies=L;      //Added no. of duplicates.
        cacheEntry->dataName = omnetDataMsg->getDataName();
        cacheEntry->realPayloadSize = omnetDataMsg->getRealPayloadSize();
        cacheEntry->dummyPayloadContent = omnetDataMsg->getDummyPayloadContent();
        cacheEntry->validUntilTime = omnetDataMsg->getValidUntilTime();
        cacheEntry->realPacketSize = omnetDataMsg->getRealPacketSize();
        cacheEntry->originatorNodeName = omnetDataMsg->getOriginatorNodeName();
        cacheEntry->destinationOriented = omnetDataMsg->getDestinationOriented();
        if (omnetDataMsg->getDestinationOriented()) {
            cacheEntry->finalDestinationNodeName = omnetDataMsg->getFinalDestinationNodeName();
        }
        cacheEntry->goodnessValue = omnetDataMsg->getGoodnessValue();
        cacheEntry->createdTime = simTime().dbl();

        cacheEntry->updatedTime = simTime().dbl();

        cacheList.push_back(cacheEntry);

        currentCacheSize += cacheEntry->realPacketSize;

        // cout << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << " --- adding cache entry, size " << currentCacheSize << "b \n";


       // EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << KSPRAYWAITROUTINGLAYER_DEBUG << " :: Adding App Generated Cache Entry :: "
        //    << cacheEntry->dataName <<"The number of copies"<< cacheEntry->copies << "\n";


//jibinEV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO <<"End of handleDataMsgFromUpperLayer loop" << "\n";
    }

    cacheEntry->lastAccessedTime = simTime().dbl();

    delete msg;


}

void KSpraywaitRoutingLayer::handleNeighbourListMsgFromLowerLayer(cMessage *msg)
{
    KNeighbourListMsg *neighListMsg = dynamic_cast<KNeighbourListMsg*>(msg);


    if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<NM>!<NC>!<" <<
                        neighListMsg->getNeighbourNameListArraySize() << ">!<CS>!<"
                            << cacheList.size() << "\n";}

    // if no neighbours or cache is empty, just return
    if (neighListMsg->getNeighbourNameListArraySize() == 0 || cacheList.size() == 0) {

        // setup sync neighbour list for the next time - only if there were some changes
        if (syncedNeighbourListIHasChanged) {
            setSyncingNeighbourInfoForNoNeighboursOrEmptyCache();
            syncedNeighbourListIHasChanged = FALSE;
        }

        delete msg;
        return;
    }

    // send summary vector messages (if appropriate) to all nodes to sync in a loop
    int i = 0;
    while (i < neighListMsg->getNeighbourNameListArraySize()) {
        string nodeMACAddress = neighListMsg->getNeighbourNameList(i);

        // get syncing info of neighbor
        SyncedNeighbour *syncedNeighbour = getSyncingNeighbourInfo(nodeMACAddress);

        // indicate that this node was considered this time
        syncedNeighbour->nodeConsidered = TRUE;

        bool syncWithNeighbour = FALSE;

        if (syncedNeighbour->syncCoolOffEndTime >= simTime().dbl()) {
            // if the sync was done recently, don't sync again until the anti-entropy interval
            // has elapsed
            syncWithNeighbour = FALSE;

        } else if (syncedNeighbour->randomBackoffStarted && syncedNeighbour->randomBackoffEndTime >= simTime().dbl()) {
            // if random backoff to sync is still active, then wait until time expires
            syncWithNeighbour = FALSE;

        } else if (syncedNeighbour->neighbourSyncing && syncedNeighbour->neighbourSyncEndTime >= simTime().dbl()) {
            // if this neighbour has started syncing with me, then wait until this neighbour finishes
            syncWithNeighbour = FALSE;

        } else if (syncedNeighbour->randomBackoffStarted && syncedNeighbour->randomBackoffEndTime < simTime().dbl()) {
            // has the random backoff just finished - if so, then my turn to start the syncing process
            syncWithNeighbour = TRUE;

        } else if (syncedNeighbour->neighbourSyncing && syncedNeighbour->neighbourSyncEndTime < simTime().dbl()) {
            // has the neighbours syncing period elapsed - if so, my turn to sync
            syncWithNeighbour = TRUE;

        } else {
            // neighbour seen for the first time (could also be after the cool off period)
            // then start the random backoff
            syncedNeighbour->randomBackoffStarted = TRUE;
            double randomBackoffDuration = uniform(1.0, maximumRandomBackoffDuration);
            syncedNeighbour->randomBackoffEndTime = simTime().dbl() + randomBackoffDuration;

            syncWithNeighbour = FALSE;

        }

        // from previous questions - if syncing required
        if (syncWithNeighbour) {

            // set the cooloff period
            syncedNeighbour->syncCoolOffEndTime = simTime().dbl() + antiEntropyInterval;

            // initialize all other checks
            syncedNeighbour->randomBackoffStarted = FALSE;
            syncedNeighbour->randomBackoffEndTime = 0.0;
            syncedNeighbour->neighbourSyncing = FALSE;
            syncedNeighbour->neighbourSyncEndTime = 0.0;

            // send summary vector (to start syncing)
            KSummaryVectorMsg *summaryVectorMsg = makeSummaryVectorMessage();
            summaryVectorMsg->setDestinationAddress(nodeMACAddress.c_str());
            send(summaryVectorMsg, "lowerLayerOut");

            if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LO>!<SVM>!<"
                << summaryVectorMsg->getSourceAddress() << ">!<" << summaryVectorMsg->getDestinationAddress()
                << ">!<CE>!<" << summaryVectorMsg->getMessageIDHashVectorArraySize() << ">!<"
                << summaryVectorMsg->getByteLength() << "\n";}

        }

        i++;
    }

    // setup sync neighbour list for the next time
    setSyncingNeighbourInfoForNextRound();

    // synched neighbour list must be updated in next round
    // as there were changes
    syncedNeighbourListIHasChanged = TRUE;

    // delete the received neighbor list msg
    delete msg;


}

void KSpraywaitRoutingLayer::handleDataMsgFromLowerLayer(cMessage *msg)
{
    KDataMsg *omnetDataMsg = dynamic_cast<KDataMsg*>(msg);
    bool found;



    if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LI>!<DM>!<" << omnetDataMsg->getSourceAddress() << ">!<"
        << omnetDataMsg->getDestinationAddress() << ">!<" << omnetDataMsg->getDataName() << ">!<" << omnetDataMsg->getGoodnessValue() << ">!<"
        << omnetDataMsg->getByteLength() << "\n";}

    // if destination oriented data sent around and this node is the destination
    // or if maximum hop count is reached
    // then cache or else don't cache
    bool cacheData = TRUE;
    if ((omnetDataMsg->getDestinationOriented()
         && strstr(getParentModule()->getFullName(), omnetDataMsg->getFinalDestinationNodeName()) != NULL)
        || omnetDataMsg->getHopCount() >= maximumHopCount) {

          if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<"<<"THE MESSAGE IN DESTINATION"<<">!<"<< omnetDataMsg->getSourceAddress()<<">!<"<<omnetDataMsg->getDataName() <<">!<"<<omnetDataMsg->getFinalDestinationNodeName()<<">!<"<<getParentModule()->getFullName()<<"\n";}

        cacheData = FALSE;
    }

    if(cacheData) {

        // insert/update cache
        CacheEntry *cacheEntry;
        list<CacheEntry*>::iterator iteratorCache;
        found = FALSE;
        iteratorCache = cacheList.begin();
        while (iteratorCache != cacheList.end()) {
            cacheEntry = *iteratorCache;
            if (cacheEntry->dataName == omnetDataMsg->getDataName()) {
                found = TRUE;
                break;
            }

            iteratorCache++;
        }

        if (!found) {

            // apply caching policy if limited cache and cache is full
            if (maximumCacheSize != 0
                && (currentCacheSize + omnetDataMsg->getRealPacketSize()) > maximumCacheSize
                && cacheList.size() > 0) {
                iteratorCache = cacheList.begin();
                advance(iteratorCache, 0);
                CacheEntry *removingCacheEntry = *iteratorCache;
                iteratorCache = cacheList.begin();
                while (iteratorCache != cacheList.end()) {
                    cacheEntry = *iteratorCache;
                    if (cacheEntry->validUntilTime < removingCacheEntry->validUntilTime) {
                        removingCacheEntry = cacheEntry;
                    }if (found) {
        send(msg, "upperLayerOut");

        if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<UO>!<DM>!<" << omnetDataMsg->getSourceAddress() << ">!<"
            << omnetDataMsg->getDestinationAddress() << ">!<" << omnetDataMsg->getDataName() << ">!<" << omnetDataMsg->getGoodnessValue() << ">!<"
            << omnetDataMsg->getByteLength() << "\n";}

    }
                    iteratorCache++;
                }
                currentCacheSize -= removingCacheEntry->realPacketSize;
                cacheList.remove(removingCacheEntry);
                delete removingCacheEntry;

            }

            cacheEntry = new CacheEntry;

            cacheEntry->messageID = omnetDataMsg->getMessageID();
			cacheEntry->copies = omnetDataMsg->getDuplicates();    //extracting parameter Duplicates from message.
            cacheEntry->dataName = omnetDataMsg->getDataName();
            cacheEntry->realPayloadSize = omnetDataMsg->getRealPayloadSize();
            cacheEntry->dummyPayloadContent = omnetDataMsg->getDummyPayloadContent();
            cacheEntry->validUntilTime = omnetDataMsg->getValidUntilTime();
            cacheEntry->realPacketSize = omnetDataMsg->getRealPacketSize();
            cacheEntry->originatorNodeName = omnetDataMsg->getOriginatorNodeName();
            cacheEntry->destinationOriented = omnetDataMsg->getDestinationOriented();
            if (omnetDataMsg->getDestinationOriented()) {
                cacheEntry->finalDestinationNodeName = omnetDataMsg->getFinalDestinationNodeName();

                if(strstr(cacheEntry->finalDestinationNodeName.c_str(),getParentModule()->getFullName()) != NULL)
                {
                  if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<"<<"THE MESSAGE IN DESTINATION"<<">!<"<<omnetDataMsg->getFinalDestinationNodeName()<< "\n";}

                }

            }
            cacheEntry->goodnessValue = omnetDataMsg->getGoodnessValue();
            cacheEntry->createdTime = simTime().dbl();
            cacheEntry->updatedTime = simTime().dbl();

            cacheList.push_back(cacheEntry);

            currentCacheSize += cacheEntry->realPacketSize;

        }

        cacheEntry->hopCount = omnetDataMsg->getHopCount() + 1;
        cacheEntry->lastAccessedTime = simTime().dbl();
    }

    // if registered app exist, send data msg to app
    AppInfo *appInfo = NULL;
    found = FALSE;
    list<AppInfo*>::iterator iteratorRegisteredApps = registeredAppList.begin();
    while (iteratorRegisteredApps != registeredAppList.end()) {
        appInfo = *iteratorRegisteredApps;
        if (strstr(omnetDataMsg->getDataName(), appInfo->prefixName.c_str()) != NULL) {
            found = TRUE;
            break;
        }
        iteratorRegisteredApps++;
    }
    if (found) {
      if (omnetDataMsg->getDestinationOriented())
      {
        if(strstr(getParentModule()->getFullName(), omnetDataMsg->getFinalDestinationNodeName()) != NULL)

        {
          send(msg, "upperLayerOut");
          if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<UO>!<DM>!<" << omnetDataMsg->getSourceAddress() << ">!<"
              << omnetDataMsg->getDestinationAddress() << ">!<" << omnetDataMsg->getDataName() << ">!<" << omnetDataMsg->getGoodnessValue() << ">!<"
              << omnetDataMsg->getByteLength() << "\n";}
        }
        else
        {
          delete msg;
        }
        }
      else
        {
          send(msg, "upperLayerOut");
          if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<UO>!<DM>!<" << omnetDataMsg->getSourceAddress() << ">!<"
              << omnetDataMsg->getDestinationAddress() << ">!<" << omnetDataMsg->getDataName() << ">!<" << omnetDataMsg->getGoodnessValue() << ">!<"
              << omnetDataMsg->getByteLength() << "\n";}

          }


    } else {
        delete msg;
    }

}



void KSpraywaitRoutingLayer::handleSummaryVectorMsgFromLowerLayer(cMessage *msg)
{



	KSummaryVectorMsg *summaryVectorMsg = dynamic_cast<KSummaryVectorMsg*>(msg);

    if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LI>!<SVM>!<" << summaryVectorMsg->getSourceAddress() << ">!<"
        << summaryVectorMsg->getDestinationAddress() << ">!<" << summaryVectorMsg->getByteLength() << "\n";}

    // when a summary vector is received, it means that the neighbour started the syncing
    // so send the data request message with the required data items


    // check and build a list of missing data items
    string messageID;
    vector<string> selectedMessageIDList;
    int i = 0;
    while (i < summaryVectorMsg->getMessageIDHashVectorArraySize()) {
        messageID = summaryVectorMsg->getMessageIDHashVector(i);

        // see if data item exist in cache
        CacheEntry *cacheEntry;
        list<CacheEntry*>::iterator iteratorCache;
        bool found = FALSE;
        iteratorCache = cacheList.begin();
        while (iteratorCache != cacheList.end()) {
            cacheEntry = *iteratorCache;
            if (cacheEntry->messageID == messageID) {
                found = TRUE;
                break;
            }

            iteratorCache++;
        }

        if (!found) {
            selectedMessageIDList.push_back(messageID);
        }
        i++;
    }

    // build a KDataRequestMsg with missing data items (i.e.,  message IDs)
    KDataRequestMsg *dataRequestMsg = new KDataRequestMsg();
    dataRequestMsg->setSourceAddress(ownMACAddress.c_str());
    dataRequestMsg->setDestinationAddress(summaryVectorMsg->getSourceAddress());
   dataRequestMsg->setOriginatorNodeName(getParentModule()->getFullName());

    int realPacketSize = 6 + 6 + (selectedMessageIDList.size() * KSPRAYWAITROUTINGLAYER_MSG_ID_HASH_SIZE);
	//IMP EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO <<"real packet size:"<<realPacketSize<<"\n";
    dataRequestMsg->setRealPacketSize(realPacketSize);
    dataRequestMsg->setByteLength(realPacketSize);
    dataRequestMsg->setMessageIDHashVectorArraySize(selectedMessageIDList.size());
    i = 0;
    vector<string>::iterator iteratorMessageIDList;
    iteratorMessageIDList = selectedMessageIDList.begin();
    while (iteratorMessageIDList != selectedMessageIDList.end()) {
        messageID = *iteratorMessageIDList;

        dataRequestMsg->setMessageIDHashVector(i, messageID.c_str());

        i++;
        iteratorMessageIDList++;
    }

    send(dataRequestMsg, "lowerLayerOut");

    if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LO>!<DRM>!<" << dataRequestMsg->getSourceAddress() << ">!<"
        << dataRequestMsg->getDestinationAddress() << ">!<" << dataRequestMsg->getByteLength() << "\n";}


    // cancel the random backoff timer (because neighbour started syncing)
    string nodeMACAddress = summaryVectorMsg->getSourceAddress();
    SyncedNeighbour *syncedNeighbour = getSyncingNeighbourInfo(nodeMACAddress);
    syncedNeighbour->randomBackoffStarted = FALSE;
    syncedNeighbour->randomBackoffEndTime = 0.0;

    // second - start wait timer until neighbour has finished syncing
    syncedNeighbour->neighbourSyncing = TRUE;
    double delayPerDataMessage = 0.5; // assume 500 milli seconds per data message
    syncedNeighbour->neighbourSyncEndTime = simTime().dbl() + (selectedMessageIDList.size() * delayPerDataMessage);

    // synched neighbour list must be updated in next round
    // as there were changes
    syncedNeighbourListIHasChanged = TRUE;


    delete msg;
}

void KSpraywaitRoutingLayer::handleDataRequestMsgFromLowerLayer(cMessage *msg)
{
    KDataRequestMsg *dataRequestMsg = dynamic_cast<KDataRequestMsg*>(msg);


    if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LI>!<DRM>!<" << dataRequestMsg->getSourceAddress() << ">!<"
        << dataRequestMsg->getDestinationAddress() << ">!<" << dataRequestMsg->getByteLength() << "\n";}

    int i = 0;
    while (i < dataRequestMsg->getMessageIDHashVectorArraySize()) {
        string messageID = dataRequestMsg->getMessageIDHashVector(i);

        CacheEntry *cacheEntry;
        list<CacheEntry*>::iterator iteratorCache;
        bool found = FALSE;
        iteratorCache = cacheList.begin();
        while (iteratorCache != cacheList.end()) {
            cacheEntry = *iteratorCache;
            if (cacheEntry->messageID == messageID) {
                found = TRUE;
                break;
            }

            iteratorCache++;
        }

        if (found) {

						bool Directtrans=FALSE;

						KDataMsg *dataMsg = new KDataMsg();

            dataMsg->setSourceAddress(ownMACAddress.c_str());
            dataMsg->setDestinationAddress(dataRequestMsg->getSourceAddress());
            dataMsg->setDataName(cacheEntry->dataName.c_str());
            dataMsg->setDummyPayloadContent(cacheEntry->dummyPayloadContent.c_str());
            dataMsg->setValidUntilTime(cacheEntry->validUntilTime);
            dataMsg->setRealPayloadSize(cacheEntry->realPayloadSize);
			//code specific to spray and wait
			newcopies=cacheEntry->copies;
			//EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO <<"the message ID:"<<cacheEntry->messageID<<"The no. of copies the node has"<<newcopies<< "\n";
			if(newcopies==1)
			{
				Directtrans=TRUE;
			dataMsg->setDuplicates(1);
		//EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO <<"DIRECT_TRANSMISSION_ENABLED"<< "\n";
			}
			// else following code.

            int realPacketSize = 6 + 6 + 2 + cacheEntry->realPayloadSize + 4 + 6 + 1+1;
            dataMsg->setRealPacketSize(realPacketSize);
            dataMsg->setByteLength(realPacketSize);
            dataMsg->setOriginatorNodeName(cacheEntry->originatorNodeName.c_str());
            dataMsg->setDestinationOriented(cacheEntry->destinationOriented);
            if (cacheEntry->destinationOriented) {
                dataMsg->setFinalDestinationNodeName(cacheEntry->finalDestinationNodeName.c_str());
            }
            dataMsg->setMessageID(cacheEntry->messageID.c_str());
            dataMsg->setHopCount(cacheEntry->hopCount);
			dataMsg->setGoodnessValue(cacheEntry->goodnessValue);



			if(!Directtrans)
			{
				//transmit message new copies times  is delay required ???
				            //for (int i=0;i<newcopies;i++)
			newcopies=(double)newcopies/2.0;
			newcopies=floor(newcopies);
			dataMsg->setDuplicates(int(newcopies));
			///update the cache
			cacheEntry->copies=(cacheEntry->copies)-newcopies;
			//EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO <<"the message ID:"<<cacheEntry->messageID<<"The no. of copies in 	cache:"<<cacheEntry->copies<<"The no. of copies going to send:"<<newcopies<< "\n";
            // check KOPSMsg.msg on sizing mssages


					send(dataMsg, "lowerLayerOut");
          if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LO>!<DM>!<" << dataMsg->getSourceAddress() << ">!<"
              << dataMsg->getDestinationAddress() << ">!<" << dataMsg->getByteLength() << ">!<" << dataMsg->getDataName() << ">!<"<<dataMsg->getDuplicates()<< "\n";}


	if(strstr(cacheEntry->finalDestinationNodeName.c_str(), dataRequestMsg->getOriginatorNodeName()) != NULL)
				{




					//code to remove message from cache is added

					CacheEntry *removingCacheEntry;
					removingCacheEntry = cacheEntry;

					currentCacheSize -= removingCacheEntry->realPacketSize;

					cacheList.remove(removingCacheEntry);
					delete removingCacheEntry;


				}

			}

		    else{

	//compare the source adress and destination address in cache if matches send to destination


				if(strstr(cacheEntry->finalDestinationNodeName.c_str(), dataRequestMsg->getOriginatorNodeName()) != NULL)
				{

					send(dataMsg, "lowerLayerOut");

          if (logging) {EV_INFO << KSPRAYWAITROUTINGLAYER_SIMMODULEINFO << ">!<" << ownMACAddress << ">!<LO>!<DM>!<" << dataMsg->getSourceAddress() << ">!<"
              << dataMsg->getDestinationAddress() << ">!<" << dataMsg->getByteLength() << ">!<" << dataMsg->getDataName() << ">!<"<<dataMsg->getDuplicates()<< "\n";}


					//code to remove message from cache is added

					CacheEntry *removingCacheEntry;
					removingCacheEntry = cacheEntry;

					currentCacheSize -= removingCacheEntry->realPacketSize;

					cacheList.remove(removingCacheEntry);
					delete removingCacheEntry;


				}

			}



        }

        i++;
    }

    delete msg;


}

KSpraywaitRoutingLayer::SyncedNeighbour* KSpraywaitRoutingLayer::getSyncingNeighbourInfo(string nodeMACAddress)
{
    // check if sync entry is there

    SyncedNeighbour *syncedNeighbour = NULL;
    list<SyncedNeighbour*>::iterator iteratorSyncedNeighbour;
    bool found = FALSE;
    iteratorSyncedNeighbour = syncedNeighbourList.begin();
    while (iteratorSyncedNeighbour != syncedNeighbourList.end()) {
        syncedNeighbour = *iteratorSyncedNeighbour;
        if (syncedNeighbour->nodeMACAddress == nodeMACAddress) {
            found = TRUE;
            break;
        }

        iteratorSyncedNeighbour++;
    }

    if (!found) {

        // if sync entry not there, create an entry with initial values
        syncedNeighbour = new SyncedNeighbour;

        syncedNeighbour->nodeMACAddress = nodeMACAddress.c_str();
        syncedNeighbour->syncCoolOffEndTime = 0.0;
        syncedNeighbour->randomBackoffStarted = FALSE;
        syncedNeighbour->randomBackoffEndTime = 0.0;
        syncedNeighbour->neighbourSyncing = FALSE;
        syncedNeighbour->neighbourSyncEndTime = 0.0;
        syncedNeighbour->nodeConsidered = FALSE;

        syncedNeighbourList.push_back(syncedNeighbour);
    }

    return syncedNeighbour;

}

void KSpraywaitRoutingLayer::setSyncingNeighbourInfoForNextRound()
{
    // loop thru syncing neighbor list and set for next round
    list<SyncedNeighbour*>::iterator iteratorSyncedNeighbour;
    iteratorSyncedNeighbour = syncedNeighbourList.begin();

    while (iteratorSyncedNeighbour != syncedNeighbourList.end()) {
        SyncedNeighbour *syncedNeighbour = *iteratorSyncedNeighbour;

        if (!syncedNeighbour->nodeConsidered) {

            // if neighbour not considered this time, then it means the
            // neighbour was not in my neighbourhood - so init all flags and timers

            syncedNeighbour->randomBackoffStarted = FALSE;
            syncedNeighbour->randomBackoffEndTime = 0.0;
            syncedNeighbour->neighbourSyncing = FALSE;
            syncedNeighbour->neighbourSyncEndTime = 0.0;
        }

        // setup for next time
        syncedNeighbour->nodeConsidered = FALSE;

        iteratorSyncedNeighbour++;

   }


}

void KSpraywaitRoutingLayer::setSyncingNeighbourInfoForNoNeighboursOrEmptyCache()
{
    // loop thru syncing neighbor list and set for next round


    list<SyncedNeighbour*>::iterator iteratorSyncedNeighbour;
    iteratorSyncedNeighbour = syncedNeighbourList.begin();
    while (iteratorSyncedNeighbour != syncedNeighbourList.end()) {
        SyncedNeighbour *syncedNeighbour = *iteratorSyncedNeighbour;

        syncedNeighbour->randomBackoffStarted = FALSE;
        syncedNeighbour->randomBackoffEndTime = 0.0;
        syncedNeighbour->neighbourSyncing = FALSE;
        syncedNeighbour->neighbourSyncEndTime = 0.0;
        syncedNeighbour->nodeConsidered = FALSE;

        iteratorSyncedNeighbour++;
    }


}

KSummaryVectorMsg* KSpraywaitRoutingLayer::makeSummaryVectorMessage()
{

    // identify the entries of the summary vector
    vector<string> selectedMessageIDList;
    CacheEntry *cacheEntry;
    list<CacheEntry*>::iterator iteratorCache;
    iteratorCache = cacheList.begin();
    while (iteratorCache != cacheList.end()) {
        cacheEntry = *iteratorCache;
        if ((cacheEntry->hopCount + 1) < maximumHopCount) {
            selectedMessageIDList.push_back(cacheEntry->messageID);
        }

        iteratorCache++;
    }

    // make a summary vector message
    KSummaryVectorMsg *summaryVectorMsg = new KSummaryVectorMsg();
    summaryVectorMsg->setSourceAddress(ownMACAddress.c_str());
    summaryVectorMsg->setMessageIDHashVectorArraySize(selectedMessageIDList.size());
    vector<string>::iterator iteratorMessageIDList;
    int i = 0;
    iteratorMessageIDList = selectedMessageIDList.begin();
    while (iteratorMessageIDList != selectedMessageIDList.end()) {
        string messageID = *iteratorMessageIDList;

        summaryVectorMsg->setMessageIDHashVector(i, messageID.c_str());

        i++;
        iteratorMessageIDList++;
    }
    int realPacketSize = 6 + 6 + (selectedMessageIDList.size() * KSPRAYWAITROUTINGLAYER_MSG_ID_HASH_SIZE);
    summaryVectorMsg->setRealPacketSize(realPacketSize);
    summaryVectorMsg->setByteLength(realPacketSize);

    return summaryVectorMsg;


}

void KSpraywaitRoutingLayer::finish()
{

    // remove age data trigger
    cancelEvent(ageDataTimeoutEvent);
    delete ageDataTimeoutEvent;

    // clear resgistered app list
    while (registeredAppList.size() > 0) {
        list<AppInfo*>::iterator iteratorRegisteredApp = registeredAppList.begin();
        AppInfo *appInfo= *iteratorRegisteredApp;
        registeredAppList.remove(appInfo);
        delete appInfo;
    }

    // clear resgistered app list
    while (cacheList.size() > 0) {
        list<CacheEntry*>::iterator iteratorCache = cacheList.begin();
        CacheEntry *cacheEntry= *iteratorCache;
        cacheList.remove(cacheEntry);
        delete cacheEntry;
    }

    // clear synced neighbour info list
    list<SyncedNeighbour*> syncedNeighbourList;
    while (syncedNeighbourList.size() > 0) {
        list<SyncedNeighbour*>::iterator iteratorSyncedNeighbour = syncedNeighbourList.begin();
        SyncedNeighbour *syncedNeighbour = *iteratorSyncedNeighbour;
        syncedNeighbourList.remove(syncedNeighbour);
        delete syncedNeighbour;
    }
}
