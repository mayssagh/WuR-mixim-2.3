
#include "Controller.h"
#include <omnetpp.h>
#include <cassert>
#include "FindModule.h"
#include "BaseDecider.h"
#include "BasePhyLayer.h"
#include <BasePhyLayer.h>
#include <MacToPhyInterface.h>
//#include "SensorAppLayerWuR.h"
//#include "DualRadio.h"

Define_Module(Controller);

void Controller::initialize(int stage) {
    BaseLayer::initialize(stage);
    if (stage == 0) {

        dual_to_phy = FindModule<MacToPhyInterface*>::findSubModule(getParentModule());
        startsOn = getParentModule()->par("startsOn").boolValue();
        if(startsOn) {
            status = AWAKE_PCA;

        } else {

            status = SLEEPING_PCA;

        }

        startsOnCSMA = getParentModule()->par("startsOnCSMA").boolValue();
        if(startsOnCSMA) {

             statusCSMA = AWAKE_CSMA;
        } else {

            statusCSMA = SLEEPING_CSMA;
                }

        cModule* host = findHost();
        if (host == 0) {
            throw cRuntimeError("Hostpointer is NULL!");
        }
        hostId = host->getId();

        delayOffToOn = par("delayOffToOn").doubleValue();
        delaySleepToOn = par("delaySleepToOn").doubleValue();
        if (delayOffToOn > 0 || delaySleepToOn > 0) {
            turnOnTimer = new cMessage("turnOnTimer", TurnOnTimer);
            turnOnTimerCSMA = new cMessage("turnOnTimerCSMA", TurnOnTimerCSMA);
        } else {
            turnOnTimer = 0;
            turnOnTimerCSMA = 0;
        }

        internalState = Controller::NONE;
    }

}


void Controller::finish()
{
    cancelAndDelete(turnOnTimer);
    cancelAndDelete(turnOnTimerCSMA);
    BaseLayer::finish();
}

bool Controller::powerOnPCA()
{
    if(status == IControl::AWAKE_PCA ) {
        throw cRuntimeError("Trying to turn on a NIC that is already ON");
    }

    if (internalState != Controller::NONE) {

        throw cRuntimeError("Trying to turn on a nic while internal "
                "state not NONE (internalState = %d)", internalState);
    }
    assert(status == IControl::SLEEPING_PCA);
    if (delayOffToOn > 0) {
        assert(turnOnTimer && !turnOnTimer->isScheduled());
        scheduleAt(simTime() + delayOffToOn, turnOnTimer);
        internalState = Controller::WAITING_FOR_OFFTOON_TIMER;
    } else {
        internalState = Controller::WAITING_FOR_ON;
        sendControlDown(new cMessage("POWER_ON_PCA", IControl::POWER_ON_PCA));
        EV<< "Nic Controller send down POWER_ON_PCA message" << endl;
   }
    return true;

}


bool Controller::powerOffPCA()
{
    if(status == IControl::SLEEPING_PCA) {
        throw cRuntimeError("Trying to turn off a nic that is already POWERED_OFF");
    }
    if (turnOnTimer && turnOnTimer->isScheduled()) {
        cancelEvent(turnOnTimer);
    }
    internalState = Controller::WAITING_FOR_OFF;
    status = IControl::SLEEPING_PCA;
    // When going to sleep or turning off, we change the state immediately and
    sendControlDown(new cMessage("POWER_OFF_PCA", IControl::POWER_OFF_PCA));
    return true;
}

bool Controller::powerOnCSMA()
{
    if(statusCSMA == IControl::AWAKE_CSMA ) {
        throw cRuntimeError("Trying to turn on a NIC that is already ON");
    }

    if (internalState != Controller::NONE) {

        throw cRuntimeError("Trying to turn on a nic while internal "
                "state not NONE (internalState = %d)", internalState);
    }
    assert(statusCSMA == IControl::SLEEPING_CSMA);
    if (delayOffToOn > 0) {
        assert(turnOnTimerCSMA && !turnOnTimerCSMA->isScheduled());
        scheduleAt(simTime() + delayOffToOn, turnOnTimerCSMA);
        internalState = Controller::WAITING_FOR_OFFTOON_TIMER_CSMA;
    } else {
        internalState = Controller::WAITING_FOR_ON_CSMA;
        sendControlDown(new cMessage("POWER_ON_CSMA", IControl::POWER_ON_CSMA));
        EV<< "Nic Controller send down POWER ON CSMA message" << endl;
    }
    return true;
}

bool Controller::powerOffCSMA()
{
    if(statusCSMA == IControl::SLEEPING_CSMA) {
        throw cRuntimeError("Trying to turn off a nic that is already TURNED_OFF");
    }
    if ( turnOnTimerCSMA && turnOnTimerCSMA->isScheduled()) {
        cancelEvent(turnOnTimerCSMA);
    }
    internalState = Controller::WAITING_FOR_OFF_CSMA;
    //statusCSMA = IControl::TURNED_OFF;
    statusCSMA = IControl::SLEEPING_CSMA;
    // When going to sleep or turning off, we change the state immediately
    sendControlDown(new cMessage("POWER_OFF_CSMA", IControl::POWER_OFF_CSMA));
    return true;
}


// ----------------- Protected member functions -------------------------------

void Controller::handleSelfMsg(cMessage* msg)
{
    switch (msg->getKind()) {
   case TurnOnTimer :
   if (msg != this->turnOnTimer) {
        throw cRuntimeError("unknown message (%s) received by Controller",
                msg->getFullName());
    }
    if (internalState == Controller::WAITING_FOR_OFFTOON_TIMER ||
            internalState == Controller::WAITING_FOR_SLEEPTOON_TIMER ) {
        internalState = Controller::WAITING_FOR_ON;
        sendControlDown(new cMessage("POWER_ON_PCA", IControl::POWER_ON_PCA));
        EV<< "Nic Controller send down TURN ON message" << endl;
    }
     else {
        throw cRuntimeError("State error: turnOnTimer expired in internalState=%d, "
                "status=%d", internalState, this->status );
    }
   break;
    case TurnOnTimerCSMA :
    if (msg != this->turnOnTimerCSMA) {
          throw cRuntimeError("unknown message (%s) received by NicController",
              msg->getFullName());
            }
     if (internalState == Controller::WAITING_FOR_OFFTOON_TIMER_CSMA ||
                 internalState == Controller::WAITING_FOR_SLEEPTOON_TIMER_CSMA ) {
             internalState = Controller::WAITING_FOR_ON_CSMA;
             sendControlDown(new cMessage("POWER_ON_CSMA", IControl::POWER_ON_CSMA));
             EV<< "Nic Controller send down TURN ON CSMA message" << endl;
         }
     else {
             throw cRuntimeError("State error: turnOnTimer expired in internalState=%d, "
                     "status=%d", internalState, this->status );

        }
        break;
}
}


/** Simple redirection of data messages */
void Controller::handleUpperMsg(cMessage* msg)
{
    EV << "Radio_State_Transition_Announcement " << msg->getName() << " in " <<  status << "\n";

    if (isAwakePCA() || isAwakeCsma()) {
        sendDown(msg);
        EV <<"ppkt sent down from NicController to MAC" << endl;
    } else {
        throw cRuntimeError("Trying to send a message down but NIC is not ON");
    }
}

void Controller::sendDataDown(cMessage *msg) {

    send(msg, lowerLayerOut);
}

/** Simple redirection of data messages */
void Controller::handleLowerMsg(cMessage* msg)
{
    EV << "Radio_State_Transition_Announcement " << msg->getName() << " in " <<  status << "\n";
    if (isAwakePCA() || isAwakeCsma()) {
        sendUp(msg);
    } else {
       throw cRuntimeError("Trying to send a message up but NIC is not ON");
    }
}

void Controller::handleLowerControl(cMessage* msg)
{
    //cMessage * m;
    switch(msg->getKind()) {
    case IControl::AWAKE_PCA:
        ev << "Controller is ON" << endl;
        sendControlUp(msg);
        if (internalState != Controller::WAITING_FOR_ON) {
            delete msg;
            throw cRuntimeError("Received TURNED_ON from MAC but not WAITING_FOR_ON");
        }
        status = IControl::AWAKE_PCA;
        internalState = Controller::NONE;
        //delete msg;
        break;
    case IControl::SLEEPING_PCA:
        ev << "Controller is OFF" << endl;
        sendControlUp(msg);
        if (internalState != Controller::WAITING_FOR_OFF) {
            delete msg;
            throw cRuntimeError("Received TURNED_OFF from MAC but not WAITING_FOR_OFF");
        }
        status = IControl::SLEEPING_PCA;
        internalState = Controller::NONE;
        //m=new cMessage("timer-backoff");
        //m->setName("RADIO_SWITCHED");
        //m->setKind(RADIO_SWITCHED);

        //sendControlUp(new cMessage("RADIO_SWITCHED", DualRadio::RADIO_SWITCHED));
        sendControlUp(new cMessage("RADIO_SWITCHED", RADIO_SWITCHED));
        //sendControlUp(m);

        break;

    case IControl::AWAKE_CSMA:
            ev << "Controller is ON" << endl;
            sendControlUp(msg);
            if (internalState != Controller::WAITING_FOR_ON_CSMA) {
                delete msg;
                throw cRuntimeError("Received AWAKE_CSMA from MAC but not WAITING_FOR_ON");
            }
            statusCSMA = IControl::AWAKE_CSMA;
            internalState = Controller::NONE;

            //delete msg;
            break;
   case IControl::SLEEPING_CSMA:
            ev << "Controller is OFF" << endl;
            sendControlUp(msg);
            if (internalState != Controller::WAITING_FOR_OFF_CSMA) {
                delete msg;
                throw cRuntimeError("Received SLEEPING_CSMAA from MAC but not WAITING_FOR_OFF_CSMA");
            }
            statusCSMA = IControl::SLEEPING_CSMA;
            internalState = Controller::NONE;
            //m=new cMessage("timer-backoff");
            //m->setName("RADIO_SWITCHED");
            //m->setKind(RADIO_SWITCHED);
            ev << "RadioState: "<< dual_to_phy->getRadioState()<< endl;;
            sendControlUp(new cMessage("RADIO_SWITCHED_CSMA", RADIO_SWITCHED_CSMA));
            //sendControlUp(m);

            break;
    default:
        //delete msg;
        //throw cRuntimeError("Received unknown control message from lower layer!");
        EV << "Radio_State_Transition_Announcement" << msg->getName() << " in " <<  status << "\n";
        this->sendControlUp(msg);
        break;
    }
}

 void Controller::handleUpperControl(cMessage* msg)
{
     switch(msg->getKind()) {
     case IControl::POWER_ON_PCA:
         EV <<"ppkt sent down from Controller to MAC" << endl;
         powerOnPCA();
         break;
     case IControl::POWER_OFF_PCA:
         powerOffPCA();
         break;
     case IControl::POWER_ON_CSMA:
         EV <<"ppkt sent down from NicController to MAC" << endl;
         powerOnCSMA();
          break;
     case IControl::POWER_OFF_CSMA:
         powerOffCSMA();
          break;
     default:
         throw cRuntimeError("Received unknown control message");

     }

}


