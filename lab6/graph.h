/*
 * graph.h - Abstract Data Type (ADT) for directed graphs
 */

#ifndef GRAPH_H
#define GRAPH_H

typedef struct G_graph_ *G_graph;  /* The "graph" type */
typedef struct G_node_ *G_node;    /* The "node" type */

typedef struct G_nodeList_ *G_nodeList;
struct G_nodeList_ { G_node head; G_nodeList tail;};

typedef bool* G_adjSet;

/****** graph opertions ******/

/* Make a new graph */
G_graph G_Graph(void);
/* Make a new node in graph "g", with associated "info" */
G_node G_Node(G_graph g, void *info);
/* Get node count of the graph */
int G_nodecount(G_graph g);
/* Get the list of nodes belonging to "g" */
G_nodeList G_nodes(G_graph g);


/****** node opertions ******/

/* Make a new edge joining nodes "from" and "to", which must belong to the same graph */
void G_addEdge(G_node from, G_node to);
void G_biAddEdge(G_node a, G_node b);
/* Delete the edge joining "from" and "to" */
void G_rmEdge(G_node from, G_node to);
/* Get key of the node */
int G_nodekey(G_node node);
/* Get all the successors of node "n" */
G_nodeList G_succ(G_node n);
/* Get all the predecessors of node "n" */
G_nodeList G_pred(G_node n);
/* Tell if there is an edge from "from" to "to" */
bool G_goesTo(G_node from, G_node n);
/* Tell how many edges lead to or from "n" */
int G_degree(G_node n);
/* Get all the successors and predecessors of "n" */
G_nodeList G_adj(G_node n);
/* Get the "info" associated with node "n" */
void *G_nodeInfo(G_node n);


/****** G_table opertions ******/

/* The type of "tables" mapping graph-nodes to information */
typedef struct TAB_table_  *G_table;
/* Make a new table */
G_table G_empty(void);
/* Enter the mapping "node"->"value" to the table "t" */
void G_enter(G_table t, G_node node, void *value);
/* Tell what "node" maps to in table "t" */
void *G_look(G_table t, G_node node);


/****** adjSet(bool *) opertions ******/

G_adjSet G_AdjSet(int nodecnt);
// mark 2 bits
void G_addAdjSet(G_adjSet adjSet, int i, int j, int nodecnt);
void G_batchAddAdjSet(G_adjSet adjSet, G_node node, G_nodeList adjs, int nodecnt);
// check 2 bits
bool G_inAdjSet(G_adjSet adjSet, int i, int j, int nodecnt);


/****** nodeList operations *******/
/* Make a NodeList cell */
G_nodeList G_NodeList(G_node head, G_nodeList tail);
/* Tell if "a" is in the list "l" */
bool G_inNodeList(G_node a, G_nodeList l);
void removeFromNodeList(G_nodeList* nodes, G_node node);
G_nodeList unionNodeList(G_nodeList a, G_nodeList b);
G_nodeList intersectNodeList(G_nodeList a, G_nodeList b);
G_nodeList diffNodeList(G_nodeList a, G_nodeList b);

/* Show all the nodes and edges in the graph, using the function "showInfo"
    to print the name of each node */
void G_show(FILE *out, G_nodeList p, void showInfo(void *));

#endif
