/*
 * NetworkLayerWuR.cc
 *
 *  Created on: 26 août 2020
 *      Author: asus
 */

#include "NetworkLayerWuR.h"

#include <cassert>

#include "NetwControlInfo.h"
#include "BaseMacLayer.h"
#include "AddressingInterface.h"
#include "SimpleAddress.h"
#include "FindModule.h"
#include "NetwPkt_m.h"
#include "ArpInterface.h"
#include "NetwToMacControlInfo.h"
#include "IControl.h"
//#include "DualRadio.h"

Define_Module(NetworkLayerWuR);

void NetworkLayerWuR::initialize(int stage)
{
    BaseNetwLayer::initialize(stage);

    if(stage==0){
        coreDebug = par("coreDebug").boolValue();
        headerLength= par("headerLength");
        arp = FindModule<ArpInterface*>::findSubModule(findHost());
    }
    else if(stage == 1) {
        // see if there is an addressing module available
        // otherwise use module id as network address
        AddressingInterface* addrScheme = FindModule<AddressingInterface*>::findSubModule(findHost());
        if(addrScheme) {
            myNetwAddr = addrScheme->myNetwAddr(this);
        } else {
            myNetwAddr = LAddress::L3Type( getId() );
        }
        coreEV << " myNetwAddr " << myNetwAddr << std::endl;
    }
}

/**
 * Decapsulates the packet from the received Network packet
 **/
cMessage* NetworkLayerWuR::decapsMsg(netwpkt_ptr_t msg)
{
    cMessage *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());
    // delete the netw packet
    delete msg;
    return m;
}


/**
 * Encapsulates the received ApplPkt into a NetwPkt and set all needed
 * header fields.
 **/
BaseNetwLayer::netwpkt_ptr_t NetworkLayerWuR::encapsMsg(cPacket *appPkt) {
    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    coreEV <<"in encaps...\n";

    netwpkt_ptr_t pkt = new NetwPkt(appPkt->getName(), appPkt->getKind());
    pkt->setBitLength(headerLength);

    cObject* cInfo = appPkt->removeControlInfo();

    if(cInfo == NULL){
    EV << "warning: Application layer did not specifiy a destination L3 address\n"
       << "\tusing broadcast address instead\n";
    netwAddr = LAddress::L3BROADCAST;
    } else {
    coreEV <<"CInfo removed, netw addr="<< NetwControlInfo::getAddressFromControlInfo( cInfo ) << std::endl;
        netwAddr = NetwControlInfo::getAddressFromControlInfo( cInfo );
    delete cInfo;
    }

    pkt->setSrcAddr(myNetwAddr);
    pkt->setDestAddr(netwAddr);
    coreEV << " netw "<< myNetwAddr << " sending packet" <<std::endl;
    if(LAddress::isL3Broadcast( netwAddr )) {
        coreEV << "sendDown: nHop=L3BROADCAST -> message has to be broadcasted"
           << " -> set destMac=L2BROADCAST\n";
        macAddr = LAddress::L2BROADCAST;
    }
    else{
        coreEV <<"sendDown: get the MAC address\n";
        macAddr = arp->getMacAddr(netwAddr);
    }

    setDownControlInfo(pkt, macAddr);

    //encapsulate the application packet
    pkt->encapsulate(appPkt);
    coreEV <<" pkt encapsulated\n";
    return pkt;
}

/**
 * Redefine this function if you want to process messages from lower
 * layers before they are forwarded to upper layers
 *
 *
 * If you want to forward the message to upper layers please use
 * @ref sendUp which will take care of decapsulation and thelike
 **/
void NetworkLayerWuR::handleLowerMsg(cMessage* msg)
{
    //if (msg->getKind() == DualRadio::ACK){
    if (msg->getKind() == ACK || msg->getKind() == ACK_CSMA ){
            EV << " handling packet from " <<
            send(msg, upperLayerOut);
        }
    else {
    netwpkt_ptr_t m = static_cast<netwpkt_ptr_t>(msg);
    coreEV << " handling packet from " << m->getSrcAddr() << std::endl;
    sendUp(decapsMsg(m));}
}

/**
 * Redefine this function if you want to process messages from upper
 * layers before they are send to lower layers.
 *
 * For the BaseNetwLayer we just use the destAddr of the network
 * message as a nextHop
 *
 * To forward the message to lower layers after processing it please
 * use @ref sendDown. It will take care of anything needed
 **/
void NetworkLayerWuR::handleUpperMsg(cMessage* msg)
{
    assert(dynamic_cast<cPacket*>(msg));
    sendDown(encapsMsg(static_cast<cPacket*>(msg)));
}

/**
 * Redefine this function if you want to process control messages
 * from lower layers.
 *
 * This function currently handles one messagetype: TRANSMISSION_OVER.
 * If such a message is received in the network layer it is deleted.
 * This is done as this type of messages is passed on by the BaseMacLayer.
 *
 * It may be used by network protocols to determine when the lower layers
 * are finished sending a message.
 **/
void NetworkLayerWuR::handleLowerControl(cMessage* msg)
{
    switch (msg->getKind())
    {
    case BaseMacLayer::TX_OVER:
        delete msg;
        break;
    case IControl::AWAKE_PCA:
        sendControlUp(msg);
        break;
    case IControl::SLEEPING_PCA:
        sendControlUp(msg);
        break;
    case BaseMacLayer::WUC_START_TX:
        send(msg, upperControlOut);
        EV <<"WUC_START_TX send to up layer";
        break;
    case BaseMacLayer::WUC_START_TX_CSMA:
        send(msg, upperControlOut);
        EV <<"WUC_START_TX_CSMA send to up layer";
        break;
    case IControl::AWAKE_CSMA:
        sendControlUp(msg);
        break;
    case IControl::SLEEPING_CSMA:
        sendControlUp(msg);
        break;
    default:
        EV << "BaseNetwLayer does not handle control messages called "
           << msg->getName() << std::endl;
        delete msg;
        break;
    }
}

void NetworkLayerWuR::handleUpperControl(cMessage* msg){
   EV << msg->getName() << std::endl;
    sendControlDown(msg);

}
/**
 * Attaches a "control info" structure (object) to the down message pMsg.
 */
cObject* NetworkLayerWuR::setDownControlInfo(cMessage *const pMsg, const LAddress::L2Type& pDestAddr)
{
    return NetwToMacControlInfo::setControlInfo(pMsg, pDestAddr);
}

/**
 * Attaches a "control info" structure (object) to the up message pMsg.
 */
cObject* NetworkLayerWuR::setUpControlInfo(cMessage *const pMsg, const LAddress::L3Type& pSrcAddr)
{
    return NetwControlInfo::setControlInfo(pMsg, pSrcAddr);
}

