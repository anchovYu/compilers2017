#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

Temp_tempList FG_def(G_node n) {
    AS_instr instr = (AS_instr)G_nodeInfo(n);
    switch (instr->kind) {
        case I_OPER:
            return instr->u.OPER.dst;
        case I_LABEL:
            return NULL;
        case I_MOVE:
            return instr->u.MOVE.dst;
    }
	assert(0); /* cannot reach here */
}

Temp_tempList FG_use(G_node n) {
    AS_instr instr = (AS_instr)G_nodeInfo(n);
    switch (instr->kind) {
        case I_OPER:
            return instr->u.OPER.src;
        case I_LABEL:
            return NULL;
        case I_MOVE:
            return instr->u.MOVE.src;
    }
	assert(0); /* cannot reach here */
}

bool FG_isMove(G_node n) {
    AS_instr instr = (AS_instr)G_nodeInfo(n);
    if (instr->kind == I_MOVE)
        return TRUE;
	return FALSE;
}

static G_node findLabelNodeInList(G_nodeList nodeList, Temp_label label) {
    G_nodeList tmpList = nodeList;
    while (tmpList && tmpList->head) {
        AS_instr instr = (AS_instr)G_nodeInfo(tmpList->head);
        if (instr->kind == I_LABEL && instr->u.LABEL.label == label)
            return tmpList->head;
        tmpList = tmpList->tail;
    }
    return NULL;
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
    // TODO: what does frame do here?
    G_graph flowGraph = G_Graph();

    G_nodeList labels = NULL;   // a node list for putting labels
    G_nodeList jumps = NULL; // a node list for jump instr waiting to insert edge to target

    AS_instrList instrTmp = il;
    G_node currNode, lastNode = NULL;
    while (instrTmp && instrTmp->head) {
        AS_instr instr = instrTmp->head;
        currNode = G_Node(flowGraph, (void *)instr);
        printf("current node: %d.\n", G_nodekey(currNode));
        AS_printInstrList(stdout, AS_InstrList(instr, NULL), Temp_name());

        // if last node is not jump or is cjump, insert an edge
        if (lastNode &&
            (!AS_hasJmp((AS_instr)G_nodeInfo(lastNode)) || AS_isCndJup((AS_instr)G_nodeInfo(lastNode)))) {
            printf("add edge between from node %d to node %d.\n", G_nodekey(lastNode), G_nodekey(currNode));
            G_addEdge(lastNode, currNode);
        }

        // if current node is a label, insert it into list for future use
        if (instr->kind == I_LABEL) {
            printf("currnode is a label, insert it for future use.\n");
            labels = G_NodeList(currNode, labels);
            // labelTmp = G_NodeList(currNode, NULL);
            // labelTmp = labelTmp->tail;
        }

        // if current node is jump, insert it into list to add edge when all labels are processed
        if (AS_hasJmp(instr)) {
            printf("currnode is a jump, insert it for future process.\n");
            jumps = G_NodeList(currNode, jumps);
            // jumpTmp = G_NodeList(currNode, NULL);
            // jumpTmp = jumpTmp->tail;
        }

        lastNode = currNode;
        instrTmp = instrTmp->tail;
        printf("\n\n");

    }

    // after placing all label nodes, we can add edge from jump node to lable node
    G_nodeList jumpTmp = jumps;
    while (jumpTmp && jumpTmp->head) {
        Temp_labelList jumpLabels= ((AS_instr)G_nodeInfo(jumpTmp->head))->u.OPER.jumps->labels;
        while (jumpLabels && jumpLabels->head) {
            printf("finding label %s for node %d.\n", S_name(jumpLabels->head),  G_nodekey(jumpTmp->head));
            G_node jumpNode = findLabelNodeInList(labels, jumpLabels->head);
            if (jumpNode) {
                printf("add edge between from node %d to node %d.\n", G_nodekey(jumpTmp->head), G_nodekey(jumpNode));
                G_addEdge(jumpTmp->head, jumpNode);
            }
            // TODO: what it node == null? what to do?
            // also, it can jump to done
            jumpLabels = jumpLabels->tail;
        }
        jumpTmp = jumpTmp->tail;
    }

    return flowGraph;
}

G_nodeList FG_succ(G_node n) {
    return G_succ(n);
}
