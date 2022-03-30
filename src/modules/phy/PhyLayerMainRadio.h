

#ifndef PHYLAYERMAINRADIO_H_
#define PHYLAYERMAINRADIO_H_

#include "PhyLayerBattery.h"
#include "BaseBattery.h"
#include "IControl.h"
#include "Decider.h"
//#include "ChannelAccess.h"
#ifndef MIXIM_INET
#include "FindModule.h"
#include "BaseMobility.h"
typedef AccessModuleWrap<BaseMobility>       ChannelMobilityAccessType;
typedef ChannelMobilityAccessType::wrapType* ChannelMobilityPtrType;
#endif


class PhyLayerMainRadio : public PhyLayerBattery, public IControl {
public:
	virtual void initialize(int stage);
	//virtual void finish();

	/**
	 * Overrides how position updates are handled.  In particular we need
	 * to handle position updates differently from what is done in
	 * ChannelAccess.
	 */
	//virtual void receiveBBItem(int category, const BBItem *details, int scopeModuleId);
	virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj);

	    /**
	     * @brief Returns the host's mobility module.
	     */
	        virtual ChannelMobilityPtrType getMobilityModule()
	        {
	            return ChannelMobilityAccessType::get(this);
	        }

protected:
	/**
	 * @brief Handles messages received from the upper layer through the
	 * control gate.
	 */
	virtual void handleUpperControlMessage(cMessage* msg);

	// Overridden functions from IControl
	virtual bool powerOnPCA();
	virtual bool powerOffPCA();
	virtual bool powerOnCSMA();
	virtual bool powerOffCSMA();

	int initialRadioState;
	cObject *obj;
	simsignal_t signalID;


	void sendControlUp(cMessage * msg);



};

#endif /* PHYLAYERMAINRADIO_H_ */
