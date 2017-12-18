#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}

static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps) {
    G_enter(t, flowNode, temps);
}

static Temp_tempList lookupLiveMap(G_table t, G_node flownode) {
    return (Temp_tempList)G_look(t, flownode);
}

bool Live_inMoveList(G_node src, G_node dst, Live_moveList moves) {
    Live_moveList tmp = moves;
    while (tmp && tmp->src && tmp->dst) {
        if (tmp->src == src && tmp->dst == dst)
            return TRUE;
        tmp = tmp->tail;
    }
    return FALSE;
}

// remove the move if it is in the moveList, return true
// else return false
bool findAndRemoveFromMoveList(G_node src, G_node dst, Live_moveList* moveList) {
    Live_moveList tmp = *moveList;
    Live_moveList last = NULL;
    while (tmp && tmp->src && tmp->dst) {
        if (tmp->src == src && tmp->dst == dst) {
            if (!last)
                *moveList = (*moveList)->tail;
            else
                last->tail = tmp->tail;
            return TRUE;
        }
        last = tmp;
        tmp = tmp->tail;
    }
    return FALSE;
}


// union (a, b), deduplicate
Live_moveList unionMoveList(Live_moveList a, Live_moveList b) {
    Live_moveList res = NULL;
    Live_moveList tmp = a;
    while (tmp && tmp->src && tmp->dst) {
        res = Live_MoveList(tmp->src, tmp->dst, res);
        tmp = tmp->tail;
    }
    tmp = b;
    while (tmp && tmp->src && tmp->dst) {
        if (!Live_inMoveList(tmp->src, tmp->dst, res))
            res = Live_MoveList(tmp->src, tmp->dst, res);
        tmp = tmp->tail;
    }
    return res;
}

// intersect (a, b)
Live_moveList intersectMoveList(Live_moveList a, Live_moveList b) {
    Live_moveList res = NULL;
    Live_moveList tmp = a;
    while (tmp && tmp->src && tmp->dst) {
        if (Live_inMoveList(tmp->src, tmp->dst, b))
            res = Live_MoveList(tmp->src, tmp->dst, res);
    }
    return res;
}


Temp_temp Live_gtemp(G_node n) {
	//your code here.
	return NULL;
}

// specially for interference graph, whose node info is temp
G_node findOrCreateNodeInTempGraph(G_graph g, Temp_temp temp) {
    G_nodeList nodeList = G_nodes(g);
    while (nodeList && nodeList->head) {
        if ((Temp_temp)G_nodeInfo(nodeList->head) == temp)
            return nodeList->head;
        nodeList = nodeList->tail;
    }
    return G_Node(g, temp);
}

static void hackFP(G_graph g) {
    G_node fpNode = findOrCreateNodeInTempGraph(g, F_FP());
    G_nodeList nodes = G_nodes(g);
    while (nodes && nodes->head) {
        G_biAddEdge(fpNode, nodes->head);
        nodes = nodes->tail;
    }
}

struct Live_graph Live_liveness(G_graph flow) {
    G_nodeList flownodes = G_nodes(flow);
    G_nodeList flownodesTmp = NULL;

    // construct the live map
    G_table livein = G_empty();
    G_table liveout = G_empty();

    bool doneFlag = FALSE;
    while (doneFlag == FALSE) {
        doneFlag = TRUE;
        while (flownodesTmp && flownodesTmp->head) {
            G_node currnode = flownodesTmp->head;
            Temp_tempList lastIn = lookupLiveMap(livein, currnode);
            Temp_tempList lastOut = lookupLiveMap(liveout, currnode);

            // in[n] = use[n] + (out[n] - def[n])
            Temp_tempList currIn = Temp_unionList(FG_use(currnode),
                                                  Temp_diffList(lastOut, FG_def(currnode)));
            // out[n] = in[n+1] + in[n+2].. (n+1, n+2 are succ[n])
            Temp_tempList currOut = NULL;
            G_nodeList succs = FG_succ(currnode);
            while (succs && succs->head) {
                currOut = Temp_unionList(currOut, lookupLiveMap(livein, succs->head));
                succs = succs->tail;
            }

            // whether the liveness will update
            if (!Temp_listEqual(lastIn, currIn) || !Temp_listEqual(lastOut, currOut))
                doneFlag = FALSE;

            // update liveness
            enterLiveMap(livein, currnode, currIn);
            enterLiveMap(liveout, currnode, currOut);

            flownodesTmp = flownodesTmp->tail;
        }
    }

    struct Live_graph lg;
    lg.moves = NULL;
    // construct the interference graph
    G_graph interference = G_Graph();
    flownodesTmp = flownodes;
    while (flownodesTmp && flownodesTmp->head) {
        G_node currnode = flownodesTmp->head;
        Temp_tempList defs = FG_def(currnode);
        Temp_tempList outs = lookupLiveMap(liveout, currnode);
        Temp_tempList tmpdefs = defs;
        Temp_tempList tmpouts = outs;
        if (!FG_isMove(currnode)) {
            while (tmpdefs && tmpdefs->head) {
                G_node def = findOrCreateNodeInTempGraph(interference, tmpdefs->head);
                while (tmpouts && tmpouts->head) {
                    G_node out = findOrCreateNodeInTempGraph(interference, tmpouts->head);
                    G_biAddEdge(def, out);
                    tmpouts = tmpouts->tail;
                }
                tmpdefs = tmpdefs->tail;
            }
        } else {
            // there should only be one def and one use
            G_node def = findOrCreateNodeInTempGraph(interference, tmpdefs->head);
            G_node use = findOrCreateNodeInTempGraph(interference, FG_use(currnode)->head);
            lg.moves = Live_MoveList(use, def, lg.moves);
            while (tmpouts && tmpouts->head) {
                G_node out = findOrCreateNodeInTempGraph(interference, tmpouts->head);
                if (out != use)
                    G_biAddEdge(def, out);
                tmpouts = tmpouts->tail;
            }
        }

        flownodesTmp = flownodesTmp->tail;
    }
    hackFP(interference);
    lg.graph = interference;
	return lg;
}
