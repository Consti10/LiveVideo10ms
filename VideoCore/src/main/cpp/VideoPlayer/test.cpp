#include <UDPSender.h>
#include <atomic>

static void fillBufferWithRandomData(std::vector<uint8_t>& data){
    const std::size_t size=data.size();
    for(std::size_t i=0;i<size;i++){
        data[i] = rand() % 255;
    }
}

// Create a buffer filled with random data of size sizeByes
std::vector<uint8_t> createRandomDataBuffer(const ssize_t sizeBytes){
  std::vector<uint8_t> buf(sizeBytes);
  fillBufferWithRandomData(buf);
  return buf;
}
// same as above but return shared ptr
std::shared_ptr<std::vector<uint8_t>> createRandomDataBuffer2(const ssize_t sizeBytes){
  auto buf=std::make_shared<std::vector<uint8_t>>(sizeBytes);
  fillBufferWithRandomData(*buf);
  return buf;
}

struct PacketInfoData{
    uint32_t seqNr;
    std::chrono::steady_clock::time_point timestamp;
} __attribute__ ((packed));
//static_assert(sizeof(PacketInfoData)==4+8);

uint32_t currentSequenceNumber=0;

void writeSequenceNumberAndTimestamp(std::vector<uint8_t>& data){
    assert(data.size()>=sizeof(PacketInfoData));
    PacketInfoData* packetInfoData=(PacketInfoData*)data.data();
    packetInfoData->seqNr=currentSequenceNumber;
    packetInfoData->timestamp= std::chrono::steady_clock::now();
}

PacketInfoData getSequenceNumberAndTimestamp(const std::vector<uint8_t>& data){
    assert(data.size()>=sizeof(PacketInfoData));
    PacketInfoData packetInfoData;
    std::memcpy(&packetInfoData,data.data(),sizeof(PacketInfoData));
    return packetInfoData;
}

// Returns true if everyhting except the first couble of bytes (PacketInfoData) match
bool compareSentAndReceivedPacket(const std::vector<uint8_t>& sb,const std::vector<uint8_t>& rb){
    if(sb.size()!=rb.size()){
        return false;
    }
    const int result=memcmp (&sb.data()[sizeof(PacketInfoData)],&rb.data()[sizeof(PacketInfoData)],sb.size()-sizeof(PacketInfoData));
    return result==0;
}

struct Options{
    const int PACKET_SIZE=1024;
    const int WANTED_PACKETS_PER_SECOND=1024;
    const int N_PACKETS=WANTED_PACKETS_PER_SECOND*5;
    const int INPUT_PORT=6001;
    const int OUTPUT_PORT=6001;
	// Default to localhost
	const std::string DESTINATION_IP="127.0.0.1";
};

// Use this to validate received data (mutex for thread safety)
struct SentDataSave{
    std::vector<std::shared_ptr<std::vector<uint8_t>>> sentPackets;
    std::mutex mMutex;
};
SentDataSave sentDataSave{};
//AvgCalculator2 avgUDPProcessingTime{1024*1024};
AvgCalculator avgUDPProcessingTime;
std::uint32_t lastReceivedSequenceNr=0;
const bool COMPARE_RECEIVED_DATA=false;

static void validateReceivedData(const uint8_t* dataP,size_t data_length){
    const auto data=std::vector<uint8_t>(dataP,dataP+data_length);
    const auto info=getSequenceNumberAndTimestamp(data);
    const auto latency=std::chrono::steady_clock::now()-info.timestamp;
	if(latency>std::chrono::milliseconds(1)){
		MLOGD<<"XGot data"<<data_length<<" "<<info.seqNr<<" "<<MyTimeHelper::R(latency)<<"\n";
	}
    //MLOGD<<"XGot data"<<data_length<<" "<<info.seqNr<<" "<<MyTimeHelper::R(latency)<<"\n";
    // do not use the first couple of packets, system needs to ramp up first
    //if(info.seqNr>10){
        avgUDPProcessingTime.add(latency);
    //}
    if(lastReceivedSequenceNr!=0){
        const auto delta=info.seqNr-lastReceivedSequenceNr;
        if(delta!=1){
            MLOGD<<"Missing a packet though FEC "<<delta;
        }
    }
    lastReceivedSequenceNr=info.seqNr;
    if(COMPARE_RECEIVED_DATA){
       sentDataSave.mMutex.lock();
       if((info.seqNr<sentDataSave.sentPackets.size()) && sentDataSave.sentPackets.at(info.seqNr)!=nullptr){
           const auto originalPacketData=sentDataSave.sentPackets.at(info.seqNr);
           if(!compareSentAndReceivedPacket(*originalPacketData,data)){
                //Also this should never happen !
                MLOGE<<"Packets do not match !"<<"\n";
           }else{
                //MLOGD<<"Packets do match"<<"\n";
           }
           // Free memory such that we do not run out of RAM (once received,we do not need the packet a second time)
           //originalPacketData.at(info.seqNr)->reset();
       }else{
            // Should never happen
            MLOGE<<"Got probably invalid seqNr "<<info.seqNr<<" "<<sentDataSave.sentPackets.size()<<"\n";
       }
       sentDataSave.mMutex.unlock();
    }
}

static void test_latency(const Options& o){
	const std::chrono::nanoseconds TIME_BETWEEN_PACKETS=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1))/o.WANTED_PACKETS_PER_SECOND;
    // start the receiver in its own thread
	// Listening always happens on localhost
    UDPReceiver udpReceiver{nullptr,o.INPUT_PORT,"LTUdpRec",0,validateReceivedData,0};
    udpReceiver.startReceiving();
    // Wait a bit such that the OS can start the receiver before we start sending data
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    UDPSender udpSender{o.DESTINATION_IP,o.OUTPUT_PORT};
    currentSequenceNumber=0;
    avgUDPProcessingTime.reset();
    sentDataSave.sentPackets.clear();
    //

    const std::chrono::steady_clock::time_point testBegin=std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point firstPacketTimePoint=std::chrono::steady_clock::now();
    std:size_t writtenBytes=0;
    std::size_t writtenPackets=0;
    for(int i=0;i<o.N_PACKETS;i++){
        auto buff=createRandomDataBuffer2(o.PACKET_SIZE);
        // If enabled,store sent data for later validation
        if(COMPARE_RECEIVED_DATA){
            sentDataSave.mMutex.lock();
            sentDataSave.sentPackets.push_back(buff);
            sentDataSave.mMutex.unlock();
        }
		//write sequence number and timestamp after random data was created
		//(We are not interested in the latency of creating random data,even thouth it is really fas)
        writeSequenceNumberAndTimestamp(*buff);
        udpSender.mySendTo(buff->data(),buff->size());
        writtenBytes+=o.PACKET_SIZE;
        writtenPackets+=1;
        currentSequenceNumber++;
        // wait until as much time is elapsed such that we hit the target packets per seconds
        const auto timePointReadyToSendNextPacket=firstPacketTimePoint+i*TIME_BETWEEN_PACKETS;
        while(std::chrono::steady_clock::now()<timePointReadyToSendNextPacket){
            //ucomment for busy wait (no scheduler interruption)
			std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    const auto testEnd=std::chrono::steady_clock::now();
    const double testTimeSeconds=(testEnd-testBegin).count()/1000.0f/1000.0f/1000.0f;
    const double actualPacketsPerSecond=(double)o.N_PACKETS/testTimeSeconds;
    const double actualMBytesPerSecond=(double)writtenBytes/testTimeSeconds/1024.0f/1024;

   // Wait for any packet that might be still in transit
   std::this_thread::sleep_for(std::chrono::seconds(1));
   udpReceiver.stopReceiving();

   MLOGD<<"Testing took:"<<testTimeSeconds<<"\n";
   MLOGD<<"WANTED_PACKETS_PER_SECOND "<<o.WANTED_PACKETS_PER_SECOND<<" Got "<<actualPacketsPerSecond<<
   " achieved bitrate: "<<actualMBytesPerSecond<<" MB/s"<<"\n";

   MLOGD<<"Avg UDP processing time "<<avgUDPProcessingTime.getAvgReadable()<<"\n";
   //MLOGD<<"All samples "<<avgUDPProcessingTime.getAllSamplesAsString();
}


int main(int argc, char *argv[])
{
	// For testing the localhost latency just use the same udp port for input and output
	// Else you have to use different udp ports and run svpcom wfb_tx and rx accordingly
	int opt;
    int ps=1024;
    int pps=2*1024;
    int wantedTime=5; // 5 seconds
	int input_port=6001;
	int output_port=6001;
	// default localhost
	int mode=0;
    while ((opt = getopt(argc, argv, "s:p:t:i:o:m:")) != -1) {
        switch (opt) {
        case 's':
            ps = atoi(optarg);
            break;
        case 'p':
            pps = atoi(optarg);
            break;
        case 't':
            wantedTime = atoi(optarg);
            break;
		//case 'i':
		//	input_port=atoi(optarg);
		//	break;
		//case 'o':
		//	output_port=atoi(optarg);
		//	break;
		case 'm':
			mode=atoi(optarg);
			break;
        default: /* '?' */
        show_usage:
            MLOGD<<"Usage: [-s=packet size in bytes] [-p=packets per second] [-t=time to run in seconds]"
			//<<"[-i=input udp port] [-o=output udp port]"
			<<" [-m= mode 0 for sendto localhost else airpi]\n";
            return 1;
        }
    }
	// Mode test localhost
	const Options options0{ps,pps,pps*wantedTime,6001,6001,"127.0.0.1"};
	// Mode test wfb latency, data goes out port 6002 to air pi where it is transmitted via wb
	// On the ground it is received via wb and forwarded to port 6001
	const Options options1{ps,pps,pps*wantedTime,6001,6002,"192.168.0.14"};
	const Options options = (mode==0) ? options0 : options1;

    // For a packet size of 1024 bytes, 1024 packets per second equals 1 MB/s or 8 MBit/s
    // 8 MBit/s is a just enough for encoded 720p video
	MLOGD<<"Selected input: "<<options.INPUT_PORT<<"\n";
	MLOGD<<"Selected output: "<<options.DESTINATION_IP<<" OUTPUT_PORT"<<options.OUTPUT_PORT<<"\n";
	test_latency(options);

    return 0;
}
