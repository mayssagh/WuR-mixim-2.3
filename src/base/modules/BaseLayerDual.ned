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

package org.mixim.base.modules;

//
// TODO auto-generated module
//
moduleinterface BaseLayerDual
{
    parameters:
       // bool coreDebug;
        int headerLength @unit("bit"); 
     gates:
        input upperLayerIn_dual; //1
        output upperLayerOut_dual; //2
        input lowerLayerIn_dual;//3
        output lowerLayerOut_dual;//4
        input upperControlIn_dual;//5
        output upperControlOut_dual;//6
        input lowerControlIn_dual;//7
        output lowerControlOut_dual;//8
        input lowerLayerInWuR_dual; // from upper layer//9
        output upperControlOutWuR_dual; // to uppe layer//10
        input lowerControlInWuR_dual;//11
        output lowerControlOutWuR_dual; // control to upper layer//12
        input upperLayerInWuR_dual;//13
    output upperLayerOutWuR_dual ;//14
    output lowerLayerOutWuR_dual;//15
    input upperControlInWuR_dual;
    
            
}
