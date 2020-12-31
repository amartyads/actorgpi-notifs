#include "AbstractChannel.hpp"

std::vector<int64_t> AbstractChannel::lookupTable;

bool AbstractChannel::compareChannelNames(AbstractChannel* a, AbstractChannel* b)
{
    return (a->name.compare(b->name) < 0);
}

uint64_t AbstractChannel::encodeGlobID(uint64_t procNo, uint64_t actNo) //static
{
	return (procNo << 20) | actNo;
}

std::pair<uint64_t,uint64_t> AbstractChannel::decodeGlobID(uint64_t inpGlobId) //static
{
	uint64_t procNo = inpGlobId >> 20;
	uint64_t actNo = inpGlobId & ((1 << 20) - 1);
	return (std::make_pair(procNo, actNo));
}