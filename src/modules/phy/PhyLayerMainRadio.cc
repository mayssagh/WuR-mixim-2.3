
#include "PhyLayerMainRadio.h"
#include "Controller.h"
#include "BaseConnectionManager.h"

Define_Module(PhyLayerMainRadio);

// ----------------- Public member functions ----------------------------------

void PhyLayerMainRadio::initialize(int stage) {

    if(stage == 0) {
        initialRadioState = par("initialRadioState").longValue();
    }

    bool startsOn = getParentModule()->par("startsOn").boolValue();
    if (startsOn) {
        PhyLayerBattery::initialize(stage);
        status = IControl::AWAKE_PCA;
        statusCSMA = IControl::AWAKE_CSMA;
        //on = true;
    } else {

        status = IControl::SLEEPING_PCA;
        statusCSMA = IControl::SLEEPING_CSMA;
        //on = false;
        if (stage == 0) {
            PhyLayerBattery::initialize(stage);
        } else if (stage == 1) {
            PhyLayer::initialize(stage);
            registerWithBattery("physical layer", numActivities);
        }
    }
}

void PhyLayerMainRadio::receiveSignal(cComponent */*source*/, simsignal_t signalID, cObject *obj)
{
    if(signalID == mobilityStateChangedSignal) {
        ChannelMobilityPtrType const mobility = check_and_cast<ChannelMobilityPtrType>(obj);
        Coord                        pos      = mobility->getCurrentPosition();

        if(isRegistered) {
            cc->updateNicPos(getNic()->getId(), &pos);
        }
        else {

            useSendDirect = cc->registerNic(getNic(), this, &pos);
            isRegistered  = true;
        }
    }
}


// ----------------- Protected member functions -------------------------------

bool PhyLayerMainRadio::powerOnPCA()
{
    ev << "Turning PHY on" << endl;
    // Start drawing current according to the 'initialRadioState'
    simtime_t switchtime = setRadioState(initialRadioState);
    if(switchtime == -1) {

        throw cRuntimeError("Could not set radio state");
    }

    if(signalID == mobilityStateChangedSignal) {
            ChannelMobilityPtrType const mobility = check_and_cast<ChannelMobilityPtrType>(obj);
            Coord                        pos      = mobility->getCurrentPosition();

            if(isRegistered) {
                cc->updateNicPos(getNic()->getId(), &pos);
            }
            else {
                // register the nic with ConnectionManager
                // returns true, if sendDirect is used
                ev << "PhyLayerMainRadio::turnOn()" << endl;
                useSendDirect = cc->registerNic(getNic(), this, &pos);
                isRegistered  = true;
            }
        }
    isRegistered = true;


    ev << "PHY turned on" << endl;
    return true;

}

bool PhyLayerMainRadio::powerOffPCA()
{
    ev << "Turning PHY off" << endl;

    decider->finish();
    isRegistered = false;

    if(txOverTimer) {
        cancelEvent(txOverTimer);
    }
    AirFrameVector channel;
    channelInfo.getAirFrames(0, simTime(), channel);
    for(AirFrameVector::iterator it = channel.begin();
            it != channel.end(); ++it)
    {
        cancelAndDelete(*it);
    }
    channelInfo = ChannelInfo();


    simtime_t switchtime = setRadioState(MiximRadio::SLEEP);
    if(switchtime == -1) {

        throw cRuntimeError("Could not set radio state");
    }

    ev << "PHY turned off" << endl;
    return true;
}

bool PhyLayerMainRadio::powerOnCSMA()
{
    ev << "Turning PHY on" << endl;
    // Start drawing current according to the 'initialRadioState'
    simtime_t switchtime = setRadioState(initialRadioState);
    if(switchtime == -1) {

        throw cRuntimeError("Could not set radio state");
    }

    if(signalID == mobilityStateChangedSignal) {
            ChannelMobilityPtrType const mobility = check_and_cast<ChannelMobilityPtrType>(obj);
            Coord                        pos      = mobility->getCurrentPosition();

            if(isRegistered) {
                cc->updateNicPos(getNic()->getId(), &pos);
            }
            else {
                // register the nic with ConnectionManager
                // returns true, if sendDirect is used
                ev << "PhyLayerMainRadio::powerOn()" << endl;
                useSendDirect = cc->registerNic(getNic(), this, &pos);
                isRegistered  = true;
            }
        }
    isRegistered = true;


    ev << "PHY turned on" << endl;
    return true;

}

bool PhyLayerMainRadio::powerOffCSMA()
{
    ev << "Turning PHY off" << endl;

    decider->finish();
    isRegistered = false;

    if(txOverTimer) {
        cancelEvent(txOverTimer);
    }
    AirFrameVector channel;
    channelInfo.getAirFrames(0, simTime(), channel);
    for(AirFrameVector::iterator it = channel.begin();
            it != channel.end(); ++it)
    {
        cancelAndDelete(*it);
    }
    channelInfo = ChannelInfo();


    simtime_t switchtime = setRadioState(MiximRadio::SLEEP);
    if(switchtime == -1) {

        throw cRuntimeError("Could not set radio state");
    }

    ev << "PHY turned off" << endl;
    return true;
}



void PhyLayerMainRadio::handleUpperControlMessage(cMessage* msg)
{
    ev<<"PhyLayerControl received upper control "<<msg->getName()<<endl;
    switch(msg->getKind()) {
    case IControl::POWER_ON_PCA:
        if(!isAwakePCA() && !isSleepingPCA()) {
            if (!powerOnPCA()) {
                throw cRuntimeError("Could not TURN_ON the PHY");
            }
        } /*else {
            if (!wakeUp()) {
                throw cRuntimeError("Could not WAKEUP the PHY");
            }
        }*/
        status = IControl::AWAKE_PCA;
        sendControlUp(new cMessage("AWAKE_PCA", IControl::AWAKE_PCA));
        //delete msg;
        break;
    case IControl::POWER_OFF_PCA:
        if(isAwakePCA() || isSleepingPCA()) {
            if(!powerOffPCA()) {
                throw cRuntimeError("Could not TURN_OFF the PHY");
            }
        }
        status = IControl::SLEEPING_PCA;
        sendControlUp(new cMessage("SLEEPING_PCA", IControl::SLEEPING_PCA));
        delete msg;
        break;
    case IControl::POWER_ON_CSMA:
            if(!isAwakeCsma() && !isSleepingCSMA()) {
                if (!powerOnCSMA()) {
                    throw cRuntimeError("Could not TURN_ON the PHY");
                }
            }
            statusCSMA = IControl::AWAKE_CSMA;
            sendControlUp(new cMessage("AWAKE_CSMA", IControl::AWAKE_CSMA));
            //delete msg;
            break;
    case IControl::POWER_OFF_CSMA:
            if(isAwakeCsma() || isSleepingCSMA()) {
                if(!powerOffCSMA()) {
                    throw cRuntimeError("Could not TURN_OFF the PHY");
                }
            }
            statusCSMA = IControl::SLEEPING_CSMA;
            sendControlUp(new cMessage("SLEEPING_CSMA", IControl::SLEEPING_CSMA));
            delete msg;
            break;
    default:
        PhyLayerBattery::handleUpperControlMessage(msg);
        //throw cRuntimeError("Received unknown control message from upper layer!");
    }

}


void PhyLayerMainRadio::sendControlUp(cMessage *msg) {
    send(msg, upperControlOut);
}


