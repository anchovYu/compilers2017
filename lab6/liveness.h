#ifndef LIVENESS_H
#define LIVENESS_H

#include "graph.h"

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);

//bool inMoveList(G_node src, G_Node dst, Live_moveList moveList);

// remove the move if it is in the moveList, return true
// else return false
bool findAndRemoveFromMoveList(G_node src, G_node dst, Live_moveList* moveList);
// union (a, b), deduplicate
Live_moveList unionMoveList(Live_moveList a, Live_moveList b);
Live_moveList intersectMoveList(Live_moveList a, Live_moveList b);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

#endif
