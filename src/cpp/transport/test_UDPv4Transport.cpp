#include <fastrtps/transport/test_UDPv4Transport.h>

using namespace std;

namespace eprosima{
namespace fastrtps{
namespace rtps{

vector<vector<char> > test_UDPv4Transport::DropLog;
uint32_t test_UDPv4Transport::DropLogLength = 0;

test_UDPv4Transport::test_UDPv4Transport(const test_UDPv4Transport::TransportDescriptor& descriptor):
   mDropDataMessages(descriptor.dropDataMessages),
   mDropHeartbeatMessages(descriptor.dropHeartbeatMessages),
   mDropAckNackMessages(descriptor.dropAckNackMessages),
   mSequenceNumberDataMessagesToDrop(descriptor.sequenceNumberDataMessagesToDrop),
   mPercentageOfMessagesToDrop(descriptor.percentageOfMessagesToDrop)
{
   UDPv4Transport::mSendBufferSize = descriptor.sendBufferSize;
   UDPv4Transport::mReceiveBufferSize = descriptor.receiveBufferSize;
   DropLog.clear();
   DropLogLength = descriptor.dropLogLength;
}

bool test_UDPv4Transport::Send(const vector<char>& sendBuffer, Locator_t localLocator, Locator_t remoteLocator)
{
   if (PacketShouldDrop(sendBuffer)          &&
       IsOutputChannelOpen(localLocator)     &&
       sendBuffer.size() <= mSendBufferSize)
   {
      LogDrop(sendBuffer);
      return true;
   }
   else   
      return UDPv4Transport::Send(sendBuffer, localLocator, remoteLocator);
}

static bool ReadSubmessageHeader(CDRMessage_t& msg, SubmessageHeader_t& smh)
{
	if(msg.length - msg.pos < 4)
		return false;

	smh.submessageId = msg.buffer[msg.pos];msg.pos++;
	smh.flags = msg.buffer[msg.pos];msg.pos++;
	msg.msg_endian = smh.flags & BIT(0) ? LITTLEEND : BIGEND;
	CDRMessage::readUInt16(&msg, &smh.submessageLength);
	return true;
}

bool test_UDPv4Transport::PacketShouldDrop(const std::vector<char>& message)
{
   CDRMessage_t cdrMessage(message.size());;
   memcpy(cdrMessage.buffer, message.data(), message.size());
   cdrMessage.length = message.size();

   return ( (mDropDataMessages      && ContainsSubmessageOfID(cdrMessage, DATA))      ||
            (mDropAckNackMessages   && ContainsSubmessageOfID(cdrMessage, ACKNACK))   ||
            (mDropHeartbeatMessages && ContainsSubmessageOfID(cdrMessage, HEARTBEAT)) );
}

bool test_UDPv4Transport::ContainsSubmessageOfID(CDRMessage_t& cdrMessage, octet ID)
{

	if(cdrMessage.length < RTPSMESSAGE_HEADER_SIZE)
      return false;

   cdrMessage.pos =  0;
   cdrMessage.pos += 4;  // RTPS header letters
	cdrMessage.pos += 12; // RTPS version

   SubmessageHeader_t cdrSubMessageHeader;
	while (cdrMessage.pos < cdrMessage.length)
   {  
      ReadSubmessageHeader(cdrMessage, cdrSubMessageHeader);
		if (cdrMessage.pos + cdrSubMessageHeader.submessageLength > cdrMessage.length)
         return false;
      if (cdrSubMessageHeader.submessageId == ID)
         return true;

	   cdrMessage.pos += cdrSubMessageHeader.submessageLength;
   }

   return false;
}


bool test_UDPv4Transport::LogDrop(const std::vector<char>& message)
{
   if (DropLog.size() < DropLogLength)
   {
      DropLog.push_back(message);
      return true;
   }

   return false;
}

bool test_UDPv4Transport::ContainsSequenceNumberToDrop(CDRMessage_t& cdrMessage)
{
   return false;
//   SubmessageHeader_t cdrSubMessageHeader;
//
//   bool contains = false;
//   while(ReadSubmessageHeader(&cdrMessage, &cdrSubMessageHeader))
//      if (contains |= (cdrSubMessageHeader.submessageId == HEARTBEAT)) break;
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima