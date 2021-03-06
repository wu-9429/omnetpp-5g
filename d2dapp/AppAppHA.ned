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

package bigdou.d2dapp;


import inet.applications.contract.IUDPApp;


simple AppAppHA like IUDPApp
{
    parameters:
        ///////////////////////////////////////////////////
        int dc;
        int node_n;
        int print_func = default(0);
        string file_pre = default("_1");
        string mbcd = default("_mb");
        string host_name = default("car");
        string packetName = default("VoIPAppHA");
        double startTime @unit(s) = default(this.sendInterval); // application start time (start of the first packet)
        double stopTime @unit(s) = default(-1s);  // time of finishing sending, -1s means forever
        volatile double sendInterval @unit(s); // should usually be a random value, e.g. exponential(1)
        volatile int messageLength @unit(B); // length of messages to generate, in bytes
        
        
        // Receiver
        int emodel_Ie_ = default(5);
        int emodel_Bpl_ = default(10);
        int emodel_A_ = default(5);
        double emodel_Ro_ = default(93.2);
        double playout_delay @unit("s") = default(0s);
        int dim_buffer = default(20);
        
        // Sender
        int localPort = default(-1);
        int destPort = default(3000);
        string destAddress;
        int PacketSize = default(40);
        double shape_talk = default(0.824);
        double scale_talk = default(1.423);
        double shape_sil = default(1.089);
        double scale_sil = default(0.899);
        bool is_talk = default(true);
        double sampling_time @unit("s") = default(0.02s);
//        double startTime @unit("s") = default(0s);
        
        bool silences = default(true);
        
        @signal[voIPGeneratedThroughput];
        @statistic[voIPGeneratedThroughput](title="Throughput generated by the application"; unit="Bps"; source="voIPGeneratedThroughput"; record=mean,vector);

        @display("i=block/source");
        
        // Receiver
        @signal[voIPFrameLoss];
        @statistic[voIPFrameLoss](title="VoIP Frame Loss"; unit="ratio"; source="voIPFrameLoss"; record=mean);
        @signal[voIPFrameDelay];
        @statistic[voIPFrameDelay](title="VoIP Frame Delay"; unit="s"; source="voIPFrameDelay"; record=mean,vector);
        @signal[voIPPlayoutDelay];
        @statistic[voIPPlayoutDelay](title="VoIP Playout Delay"; unit="s"; source="voIPPlayoutDelay"; record=mean,);
        @signal[voIPPlayoutLoss];
        @statistic[voIPPlayoutLoss](title="VoIP Playout Loss"; unit="ratio"; source="voIPPlayoutLoss"; record=mean);
        @signal[voIPJitter];
        @statistic[voIPJitter](title="VoIP Playout Loss"; unit="s"; source="voIPJitter"; record=mean);
        @signal[voIPMos];
        @statistic[voIPMos](title="VoIP Mos Signal"; unit="MOS"; source="voIPMos"; record=mean);
        @signal[voIPTaildropLoss];
        @statistic[voIPTaildropLoss](title="VoIP Tail Drop Loss"; unit="ratio"; source="voIPTaildropLoss"; record=mean);
        @signal[voIPReceivedThroughput];
        @statistic[voIPReceivedThroughput](title="Throughput received at the application level"; unit="Bps"; source="voIPReceivedThroughput"; record=mean,vector);
    
		int ues;
    gates:
        output udpOut;
        input udpIn;
//        input hostIn[ues] @directIn;
		input hostIn @directIn;
        output hostOut @directIn;
}