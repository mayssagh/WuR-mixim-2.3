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

// MainNic" uses a submodule called "Controller" in addition to "Phy" and "mac" submodules. 
//"Controller" module is responsible for powering ON/OFF the Main Radio (MR). In fact, it forwards
//"POWER_ ON" and "POWER_ OFF" control messages, received from "Control" module,
//in the proper order to lower modules of the "MainNic".
//It also ensures that data redirection is only done when the MR is enabled. 
//"MAC" submodule is responsible for sending data and acknowledgments. 
//Data sending process can only succeed when physical submodule activates the MR after receiving a "POWER_ON" message.

package org.mixim.modules.nic;
import org.mixim.modules.nic.WirelessNic;
//import org.mixim.modules.phy.PhyLayerBattery;
import org.mixim.modules.phy.PhyLayerMainRadio;
import org.mixim.modules.mac.csmaMain2;
//import org.mixim.modules.mac.SCMWuR;
import org.mixim.modules.nic.Controller;

module MainNic2 like IWirelessNicUpperCtrl 
{
    parameters:
        string connectionManagerName = default("");   //name of the ConnectionManager module
        // power consumption from boards (at 3.3V)
       double sleepCurrent @unit(mA) = 0.02mA; // 0.021 µA  0.000021
       double rxCurrent    @unit(mA)  = 18.8 mA;
       double decodingCurrentDelta @unit(mA) = 0 mA;
       double txCurrent   @unit(mA)    = 17.4 mA;
       double setupRxCurrent  @unit(mA)   = 0.6391mA; // 0.002109 W = (ESetupPower+ESetupXtal+ETxCalibrate)/TSetupTx
       double setupTxCurrent  @unit(mA)   = 0.6391mA; // 0.002259 W = (ESetupPower+ESetupXtal+ERxCalibrate)/TSetupRx
       double rxTxCurrent   @unit(mA)   = 18.8 mA; // Upper bound
       double txRxCurrent  @unit(mA)     = 18.8 mA; // idem
        bool startsOn = default(false);
        bool startsOnCSMA = default(false);
        @display("bgb=140,303");
        
        
    gates:
        input upperLayerIn; // from upper layers
        input radioIn; // to receive AirFrames
        output upperLayerOut; // to upper layers
        output upperControlOut; // control connection
        input upperControlIn;
    submodules:
        phy: PhyLayerMainRadio {
            parameters:
                decider = xmldoc("Nic802154_TI_CC2420_Decider.xml");
                // power consumption from boards (at 3.3V)
              headerLength = 48 bit; // ieee 802.15.4                
              thermalNoise = default(-110 dBm);
        // From TI CC1100 datasheet rev. C
       
              timeSleepToRX = 0s;  //0.001792 s 
              timeSleepToTX =0 s; //0.001792
              timeRXToTX = 0s; //0.000192
              timeTXToRX = 0 s; //0.000192 s
              timeRXToSleep = 0 s;
              timeTXToSleep = 0 s;

       @display("p=70,216;i=block/process_s");
        }

        mac: csmaMain2 {
            parameters:
              rxSetupTime = 0s; //0.001792 0.000102
              bitrate = 250000 bps;
              txPower = default(43.2 mW);

       @display("p=70,148;i=block/layer");
        }
        Controller: Controller {
            parameters:
                //@display("p=70,63;i=block/cpu, #4f4f4f");
       @display("p=70,63");
        }

    connections:
        radioIn --> phy.radioIn;

        Controller.upperLayerOut --> { @display("ls=red;m=m,25,50,25,0"); } --> upperLayerOut;
        Controller.upperLayerIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- upperLayerIn;
        Controller.upperControlOut --> { @display("ls=black;m=m,75,50,75,0"); } --> upperControlOut;
        Controller.upperControlIn <-- { @display("ls=black;m=m,85,0,85,0"); } <-- upperControlIn;

        // mac to control: data goes here
        mac.upperLayerOut --> { @display("ls=red;m=m,25,50,25,0"); } --> Controller.lowerLayerIn;
        mac.upperLayerIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- Controller.lowerLayerOut;
        mac.upperControlOut --> { @display("ls=black;m=m,75,50,75,0"); } --> Controller.lowerControlIn;
        mac.upperControlIn <-- { @display("ls=black;m=m,85,0,85,0"); } <-- Controller.lowerControlOut;

        phy.upperLayerOut --> { @display("ls=red;m=m,25,50,25,0"); } --> mac.lowerLayerIn;
        phy.upperLayerIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- mac.lowerLayerOut;
        phy.upperControlOut --> { @display("ls=black;m=m,75,50,75,0"); } --> mac.lowerControlIn;
        phy.upperControlIn <-- { @display("ls=black;m=m,85,0,85,0"); } <-- mac.lowerControlOut;







}
