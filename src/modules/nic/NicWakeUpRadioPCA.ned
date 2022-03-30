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

package org.mixim.modules.nic;
import org.mixim.modules.nic.WirelessNic;
import org.mixim.modules.phy.PhyLayerBattery;
import org.mixim.modules.mac.D2MACWuR;
//
// TODO auto-generated type
//
module NicWakeUpRadioPCA like IWirelessNicUpperCtrl
{
    string connectionManagerName = default("");   //name of the ConnectionManager module
    // power consumption from boards (at 3.3V)
    double sleepCurrent @unit(mA) = 0.0035mA; // 0.021 µA
    double rxCurrent @unit(mA) = 0.008 mA;
    double decodingCurrentDelta @unit(mA) = 0 mA;
    double txCurrent @unit(mA) = 152 mA;
    double setupRxCurrent @unit(mA) = 0.6391mA; //0.0027 0.6391// 0.002109 W = (ESetupPower+ESetupXtal+ETxCalibrate)/TSetupTx
    double setupTxCurrent @unit(mA) = 0.6845mA; // 0.6845// 0.002259 W = (ESetupPower+ESetupXtal+ERxCalibrate)/TSetupRx
    double rxTxCurrent @unit(mA) = 0.0027 mA; //  18.8 0.0027 // Upper bound
    double txRxCurrent @unit(mA) = 0.0027 mA; // 18.8// idem 0.0027

    @display("bgb=166,236");
    gates:
        input upperLayerIn; // from upper layers
        input radioIn; // to receive AirFrames
        output upperLayerOut; // to upper layers
        output upperControlOut; // control connection
        input upperControlIn;
        
        
       
    submodules:
        phy: PhyLayerBattery {
            parameters:
                decider = xmldoc("NicWakeUpRadio_Decider.xml");
                //publishRSSIAlways = false;
                headerLength = 48 bit; // ieee 802.15.4   ==48bits             
                thermalNoise = -110 dBm;
                // From TI CC1100 datasheet rev. C
                timeSleepToRX = 0.001792s;  //0.001792 s 
                timeSleepToTX =0.001792 s; //0.001792
               timeRXToTX = 0.000192s; //0.000192
                timeTXToRX = 0.000192 s; //0.000192 s
                timeRXToSleep = 0 s;
                timeTXToSleep = 0 s;
                //maxTXPower    = default(100 mW);
                //timeSleepToRX = 0.000102 s;
                //timeSleepToTX = 0.000203 s;
                //timeRXToTX =   0.000192s;     //0.00021 s;
                //timeTXToRX =   0.000192s;            //0.00012 s;
                //timeRXToSleep = 0.000031 s;
              //  timeTXToSleep = 0.000032 s;
       
                // power consumption from boards
                //                sleepPower = 0.0000000693 W; // 0.021 microA * 3.3 V
                //                rxPower = 0.06204 W;   // 18.8 mA * 3.3 V
                //                txPower = 0.05742 W;   // 17.4 mA * 3.3 V
                //                rxTxPower = 0.06204 W; // Upper bound : 18.8 mA * 3.3 V
                //                txRxPower = 0.06204 W; // idem
                //                setupTxPower = 0.002109 W; // (ESetupPower+ESetupXtal+ETxCalibrate)/TSetupTx
                //                setupRxPower = 0.002259 W; // (ESetupPower+ESetupXtal+ERxCalibrate)/TSetupRx
                @display("p=65,151;i=block/process_s");

        }

        mac: D2MACWuR {
            parameters:
                rxSetupTime =0.001792 s; //0.001792 s 0.000192
                bitrate =100000 bps;  //15360
                txPower = default(1 mW);

                @display("p=65,58;i=block/layer");
        }
    connections:

        radioIn --> phy.radioIn;

        //phy.upperGateIn <-- { @display("m=m,25,0,25,0"); } <-- mac.lowerGateOut;
       // phy.upperGateOut --> { @display("m=m,73,0,50,50"); } --> mac.lowerGateIn;
        //phy.upperControlOut --> mac.lowerControlIn;
        //phy.upperControlIn <-- mac.lowerControlOut;

        //mac.upperGateOut --> upperGateOut;
        //mac.upperControlOut --> upperControlOut;
       // mac.upperGateIn <-- upperGateIn;
       // mac.upperControlIn <-- upperControlIn;
        
        mac.upperLayerOut --> { @display("ls=red;m=m,25,50,25,0"); } --> upperLayerOut;
        mac.upperLayerIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- upperLayerIn;
        mac.upperControlOut --> { @display("ls=black;m=m,75,50,75,0"); } --> upperControlOut;
       mac.upperControlIn <-- { @display("ls=black;m=m,85,0,85,0"); } <-- upperControlIn;
       
       phy.upperLayerOut --> { @display("ls=red;m=m,25,50,25,0"); } --> mac.lowerLayerIn;
        phy.upperLayerIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- mac.lowerLayerOut;
        phy.upperControlOut --> { @display("ls=black;m=m,75,50,75,0"); } --> mac.lowerControlIn;
        phy.upperControlIn <-- { @display("ls=black;m=m,85,0,85,0"); } <-- mac.lowerControlOut;
       
         //mac.upperLayerOut --> { @display("ls=red;m=m,25,50,25,0"); } --> upperLayerOut;
        // mac.upperLayerIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- upperLayerIn;
        // mac.upperControlOutWuR --> { @display("ls=black;m=m,75,50,75,0"); } --> upperControlOutWuR;
         //mac.upperControlInWuR <-- { @display("ls=black;m=m,85,0,85,0"); } <-- upperControlInWuR;

       // phy.upperGateOut --> { @display("ls=red;m=m,25,50,25,0"); } --> mac.lowerLayerIn;
       // phy.upperGateIn <-- { @display("ls=red;m=m,15,50,15,0"); } <-- mac.lowerLayerOut;
       // phy.upperControlOut --> { @display("ls=black;m=m,75,50,75,0"); } --> mac.lowerControlInWuR;
       // phy.upperControlIn <-- { @display("ls=black;m=m,85,0,85,0"); } <-- mac.lowerControlOutWuR;

}
