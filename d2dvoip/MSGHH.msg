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

packet MSGHH
{
    unsigned int IDtalk;
    unsigned int nframes;
    unsigned int IDframe;
    simtime_t timestamp;
    simtime_t arrivalTime;
    simtime_t playoutTime;
    unsigned int size;
    
    
    int posx;
    int posy;
    int type;		// Type of message
    int dataId;		// Number of the message
    int hops;		// The number of hops the message has passed
//    bool reply;		// 
//    bool forward;	// 
    
    int srcIdx;		// The id of the vehicle that generated the message
    string srcAddrs;	// The address of the vehicle that generated the message
    string srcHostName;	// The host name of the vehicle that generated the message
//    int destId;		// 
//    string destAddrs;	// 
//    string destHostName;	// 
//    int localId;	// The id of the vehicle currently forwarding the message
//    string localAddrs;	// The address of the vehicle currently forwarding the message
//    string localHostName;	// The host name of the vehicle currently forwarding the message


//    int neiPos;		// Store the number of neighbor vehicles of the vehicle that generated the message
////    int neiCap;	// 
//    int neiIds[201];	// Store the neighbor vehicle id of the vehicle that generated the message
//    double neiTimes[201];	// Stores the time when the neighboring vehicles of the vehicle that generated the message met

//    int srcPos;		// 
//    int srcIds[200];	// 

	int top_n;		// Store the number of topology
    int destPos;	// Store the number of valid vehicle IDs when the base station sends a message
    int destIdxs[500];	// Store the vehicle id that is transmitted sequentially when the base station sends a message
    int nodeHops[10];
}