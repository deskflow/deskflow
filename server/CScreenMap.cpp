#include "CScreenMap.h"
#include <assert.h>

//
// CScreenMap
//

CScreenMap::CScreenMap()
{
	// do nothing
}

CScreenMap::~CScreenMap()
{
	// do nothing
}

void					CScreenMap::addScreen(const CString& name)
{
	if (m_map.count(name) != 0) {
		assert(0 && "name already in map");	// FIXME -- throw instead
	}
	m_map.insert(std::make_pair(name, CCell()));
}

void					CScreenMap::removeScreen(const CString& name)
{
	CCellMap::iterator index = m_map.find(name);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// remove from map
	m_map.erase(index);

	// disconnect
	for (index = m_map.begin(); index != m_map.end(); ++index) {
		CCell& cell = index->second;
		for (SInt32 i = 0; i <= kLastDirection - kFirstDirection; ++i)
			if (cell.m_neighbor[i] == name) {
				cell.m_neighbor[i].erase();
			}
	}
}

void					CScreenMap::removeAllScreens()
{
	m_map.clear();
}

void					CScreenMap::connect(const CString& srcName,
								EDirection srcSide,
								const CString& dstName)
{
	// find source cell
	CCellMap::iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// connect side (overriding any previous connection)
	index->second.m_neighbor[srcSide - kFirstDirection] = dstName;
}

void					CScreenMap::disconnect(const CString& srcName,
								EDirection srcSide)
{
	// find source cell
	CCellMap::iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// disconnect side
	index->second.m_neighbor[srcSide - kFirstDirection].erase();
}

CScreenMap::const_iterator
						CScreenMap::begin() const
{
	return const_iterator(m_map.begin());
}

CScreenMap::const_iterator
						CScreenMap::end() const
{
	return const_iterator(m_map.end());
}

CString					CScreenMap::getNeighbor(const CString& srcName,
								EDirection srcSide) const
{
	// find source cell
	CCellMap::const_iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// return connection
	return index->second.m_neighbor[srcSide - kFirstDirection];
}

const char*				CScreenMap::dirName(EDirection dir)
{
	static const char* s_name[] = { "left", "right", "top", "bottom" };
	return s_name[dir - kFirstDirection];
}
