//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __MIXIM_CONTROLLER_H_
#define __MIXIM_CONTROLLER_H_

#include <omnetpp.h>
#include <BaseLayer.h>
#include "IControl.h"
#include <NetwPkt_m.h>
#include "ApplPkt_m.h"
#include "MacToPhyInterface.h"

class MacToPhyInterface;
/**
 * TODO - Generated class
 */
class Controller : public BaseLayer, public IControl
{
public:
    virtual void initialize(int stage);
    virtual void finish();

    /** @brief Returns a unique id for the nic*/
    int getNicId() const {return this->getId();}

    enum NicControlKinds {
        RADIO_SWITCHED = 5,
        DATA_MESSAGE = 1,
        RADIO_SWITCHED_CSMA = 20,

    };

protected:
    /** @brief Handler to the physical layer.*/
        MacToPhyInterface* dual_to_phy;

    /** @brief Not needed.  Throws an exception if ever called. */
    virtual void handleSelfMsg(cMessage* msg);

    /** @brief Handle messages from upper layer */
    virtual void handleUpperMsg(cMessage *msg);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage *msg);

    /** @brief Handle control messages from lower layer */
    virtual void handleLowerControl(cMessage *msg);

    /** @brief Handle control messages from upper layer */
    virtual void handleUpperControl(cMessage *msg);

    void sendDataDown(cMessage *msg);



    // Overridden functions from IControl
    virtual bool powerOnPCA();
    virtual bool powerOffPCA();
    virtual bool powerOnCSMA();
    virtual bool powerOffCSMA();
private:
    enum InternalState {NONE, WAITING_FOR_ON, WAITING_FOR_OFFTOON_TIMER, WAITING_FOR_SLEEPTOON_TIMER, WAITING_FOR_OFF, WAITING_FOR_SLEEP, WAITING_FOR_ON_CSMA, WAITING_FOR_OFFTOON_TIMER_CSMA, WAITING_FOR_SLEEPTOON_TIMER_CSMA, WAITING_FOR_OFF_CSMA, WAITING_FOR_SLEEP_CSMA};
    InternalState internalState;
    enum NicController_MSG_TYPES
           {
        TurnOnTimer,
        TurnOnTimerCSMA
           };

    /** @brief Should the NIC be turned on when created?*/
    bool startsOn;
    bool startsOnCSMA;


    int hostId;
    simtime_t delayOffToOn, delaySleepToOn;

    cMessage* turnOnTimer;
    cMessage* turnOnTimerCSMA;

    //void publishNicState();

    int destAddr;
    int myAddr;
    double headerLength;
};

#endif
