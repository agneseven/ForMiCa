//
// Created by A on 16/10/17.
//

#include "manage.h"

#include "event_list.h"
#include "Grafo.h"
#include "Coordinates.h"
#include "RandomWalk.h"
#include "mobileNode.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iterator>
#include "igraph.h"
#include <algorithm>



#define PI (3.141592653589793)
#define SPEED_OF_LIGHT  299792458.0 // m/s


extern Grafo* Topology;
igraph_t g1;
std::vector<double> totLifeTaskTime; //vector of the lifetime of each completed task
std::vector<int> jobIDLifeTaskTime; //vector of the  id of each completed task
int totGeneratedEvent; //total number of event generated during the simulation
int totcompletedTask; // variable updated with the number of completed tasks
int migration; // variable updated with the number of migrated tasks
int NOmigration; // variable updated with the number of tasks that don't migrate
int lostTask; // variable updated with the number of lost tasks because the node that was hosting them failed and it wasn't connected to any other node
int failedVertex; // variable updated with the number of failed vertex
int overcomedThreshold; //number of times the threshold has been overcomed
std::vector<int> vecThreshold; //vector with the value of all the evaluated threshold

std::default_random_engine generator(time(NULL));



//==============================================================================
//= Function to generate exponential / uniform / heavy tail distribution
//==============================================================================
double exponential(double x)
{
    double z; // Uniform random number from 0 to 1
    // Pull a uniform RV (0 < z < 1)
    do
    {
        z = ((double) rand() / RAND_MAX);
    }
    while ((z == 0) || (z == 1));
    
    return(-x * log(z));
}

double uniformDistr(double x)
{
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0,x);
    
    double number = distribution(generator);
    
    
    return(number);
}

double htDistr(double x)
{
    std::default_random_engine generator;
    std::weibull_distribution<double> distribution(0.5,x);
    
    double number = distribution(generator);
    
    return(number);
}



//===========================================================================
//= Function to generate random coordinates / random vertex id
//===========================================================================
double Coord_rand(double Border){
    double range = (Border - 0);
    double div = RAND_MAX / range;
    return 0 + (rand() / div);
}


int manage::randId(int nvertici){
    // std::cout << "rand " << std::endl;
    double range = (nvertici - 0);
    double div = RAND_MAX / range;
    return 0 + (rand() / div);

}

//===========================================================================
//= Function to generate the ID of the most powerful nodes
//===========================================================================
int preferentialIDgen(int i, int nvertici, std::vector<double> serviceTime){
    std::vector<double> listProb;
    double sumlistMigrationCost = 0;
    int n;
    for (int id = 0; id < nvertici; id++) {
        if (Topology[id].getState() == 1 && id != i && Topology[i].getNeighbours(id) == 1) {
            sumlistMigrationCost = sumlistMigrationCost + (1 / serviceTime[id]);
        }
    }

    for (int id = 0; id < nvertici; id++) {
        if (Topology[id].getState() == 1 && id != i && Topology[i].getNeighbours(id) == 1) {
            listProb.push_back((1 / serviceTime[id]) / sumlistMigrationCost);
        } else if (Topology[id].getState() == 0 || id == i || Topology[i].getNeighbours(id) == 0) {
            listProb.push_back(0);
        }
    }

    double val = (double) rand() / RAND_MAX;
    double cumulativeProbability = 0;

    for (int id = 0; id < nvertici; id++) {
        if (Topology[id].getState() == 1 && Topology[i].getNeighbours(id) == 1) {
            cumulativeProbability = cumulativeProbability + listProb[id];
            if (val < cumulativeProbability && id != i) {
                n = id;
                break;
            }
        }
    }

    return n;

}

//===========================================================================
//= Function to generate the ID of the most powerful nodes
//===========================================================================
int powerfulIDgen(int nvertici, std::vector<double> serviceTime){
    std::vector<double> listProb;
    double sumlistMigrationCost = 0;

    for (int id = 0; id < nvertici; id++) {
        if(Topology[id].getState() == 1 )
            sumlistMigrationCost = sumlistMigrationCost + (1/serviceTime[id]);
    }

    for (int id = 0; id < nvertici; id++) {
        if(Topology[id].getState() == 1){
            listProb.push_back((1/serviceTime[id])/sumlistMigrationCost);
        }
        else if(Topology[id].getState() == 0){
            listProb.push_back(0);
        }
    }

    int n;
    double val = (double)rand() / RAND_MAX;
    double cumulativeProbability = 0;

    for (int id = 0; id < nvertici; id++) {
        if(Topology[id].getState() == 1 ){
            cumulativeProbability =  cumulativeProbability + listProb[id];

            if (val < cumulativeProbability) {
                n = id;
                break;
            }
        }
    }
    return n;

}

//===========================================================================
//= Function to generate the ID according to the harmonic policy
//===========================================================================
int manage::kserverIDgen(int currentID, int nvertici, std::vector<double> serviceTime, double timeelab){
    std::vector<double> listMigrationCost;
    double sumlistMigrationCost = 0;

    for (int id = 0; id < nvertici; id++) {
        if (Topology[id].getState() == 1 && id != currentID && Topology[currentID].getNeighbours(id) == 1) {
            double transmissionCost_new = delayCalculation(currentID, id);
            double queueCost_new = serviceTime[id] * (Topology[id].getSizeQueue());
            double elaborationTime_new = timeelab;
            listMigrationCost.push_back(transmissionCost_new + queueCost_new  + elaborationTime_new);

            sumlistMigrationCost = sumlistMigrationCost + (1/listMigrationCost[id]);
        }
        else if (Topology[id].getState() == 0 || id == currentID || Topology[currentID].getNeighbours(id) == 0) {
            listMigrationCost.push_back(0);
        }
    }

    std::vector<double> listProb;


    for (int id = 0; id < nvertici; id++) {
        if (Topology[id].getState() == 1 && id != currentID && Topology[currentID].getNeighbours(id) == 1) {
            listProb.push_back((1/listMigrationCost[id])/sumlistMigrationCost);
        }
        else if (Topology[id].getState() == 0 || id == currentID || Topology[currentID].getNeighbours(id) == 0) {
            listProb.push_back(0);
        }
    }


    double val = (double)rand() / RAND_MAX;
    double cumulativeProbability = 0;

    int n;

    for (int id = 0; id < nvertici; id++) {
        if (Topology[id].getState() == 1 && Topology[currentID].getNeighbours(id) == 1) {
            cumulativeProbability = cumulativeProbability + listProb[id];

            if (val < cumulativeProbability && id != currentID) {
                n = id;
                break;
            }
        }
    }

    return n;
}





manage::manage(){
}

manage::~manage(){
}



//===========================================================================
//= Function to initialize the process parameters
//===========================================================================
int manage::initialize_process(int argc, char** argv) {

    Event *ev;
    char* modeTs;
    char* modeTd;
    int option;
    
    double Border1 = 1000.0; //dimension of the plane
    totGeneratedEvent = 0;
    failedVertex = 0;
    migration = 0;
    lostTask = 0;
    NOmigration = 0;
    totcompletedTask = 0;
    EndOfSimulation = false;
    overcomedThreshold = 0;
    CurrentTime = 0.0;

    


    //----------------------------------------------------------------------------
    

    simulationTime = atof(argv[6]); //total duration of the simulation
    numMob = atoi(argv[7]); //number of mobile devices that generate packets
    modeTs = argv[8]; //distribution of the service time of the node of the topology
    modeTd = argv[9]; //distribution of the generation time of the mobile nodes
    int speed = atoi(argv[13]); // mobile node's speed
    int seed = atoi(argv[14]); //seed
    int migrationMode = atoi(argv[15]); //migration cost on/off
    int failureMode = atoi(argv[16]); //failure on/off
    int failedPerc = atoi(argv[17]); //percentage of nodes that we want to fail
    int THRESHOLDMode = atoi(argv[18]); //threshold on/off

    const std::string uniformMode = "uniform"; // uniform distribution
    const std::string exponentialMode = "exponential"; // exponential distribution
    const std::string constantMode = "constant"; // constant distribution
    
    if(modeTs == uniformMode && modeTd == uniformMode){
        option = 0;
    }
    else if(modeTs == exponentialMode && modeTd == exponentialMode){
        option = 1;
    }
    else if(modeTs == constantMode && modeTd == constantMode){
        option = 2;
    }
    else if(modeTs == constantMode && modeTd == uniformMode){
        option = 3;
    }
    else if(modeTs == uniformMode && modeTd == constantMode){
        option = 4;
    }

    std::default_random_engine generator;

    switch(option) {
        case (0): {
            double maxUnifValueTS = atof(argv[10]);
            std::uniform_real_distribution<> distributionUnifTS(0, maxUnifValueTS);
            double maxUnifValueTD = atof(argv[11]);
            std::uniform_real_distribution<> distributionUnifTD(0, maxUnifValueTD);

            for (int i = 0; i < nvertici; i++) {
                double TSvalue = 0;
                while (TSvalue == 0) {
                    TSvalue = distributionUnifTS(generator);
                }
                serviceTime.push_back(TSvalue);
                std::cout << " mean service time " << serviceTime[i] << std::endl;
            }

            for (int i = 0; i < nvertici; i++) {
                double TDvalue = 0;
                while (TDvalue == 0) {
                    TDvalue = distributionUnifTD(generator);
                }
                arrivalTime.push_back(TDvalue);
                std::cout << " mean generation time " << arrivalTime[i] << std::endl;
            }

            break;
        }

        case (1): {
            double avgExpValueTS = atof(argv[10]);
            std::exponential_distribution<double> distributionExpTS(avgExpValueTS);
            double avgExpValueTD = atof(argv[11]);
            std::exponential_distribution<double> distributionExpTD(avgExpValueTD);
            for (int i = 0; i < nvertici; i++) {
                double TSvalue = 0;
                while (TSvalue == 0) {
                    TSvalue = distributionExpTS(generator);
                }
                serviceTime.push_back(TSvalue);
                std::cout << " mean service time " << serviceTime[i] << std::endl;
            }

            for (int i = 0; i < nvertici; i++) {
                double TDvalue = 0;
                while (TDvalue == 0) {
                    TDvalue = distributionExpTD(generator);
                }
                arrivalTime.push_back(TDvalue);
                std::cout << " mean generation time " << arrivalTime[i] << std::endl;
            }
            break;
        }
        case (2): {
            double constValueTS = atof(argv[10]);
            double constValueTD = atof(argv[11]);

            for (int i = 0; i < nvertici; i++) {
                double TSvalue = constValueTS;
                serviceTime.push_back(TSvalue);
                std::cout << " mean service time " << serviceTime[i] << std::endl;
            }
            for (int i = 0; i < nvertici; i++) {
                double TDvalue = constValueTD;
                arrivalTime.push_back(TDvalue);
                std::cout << " mean generation time " << arrivalTime[i] << std::endl;
            }
            break;

        }
        case (3): {
            double constValueTS = atof(argv[10]);
            double maxUnifValueTD = atof(argv[11]);
            std::uniform_real_distribution<> distributionUnifTD(0, maxUnifValueTD);

            for (int i = 0; i < nvertici; i++) {
                double TSvalue = constValueTS;
                serviceTime.push_back(TSvalue);
                std::cout << " mean service time " << serviceTime[i] << std::endl;
            }

            for (int i = 0; i < nvertici; i++) {
                double TDvalue = 0;
                while (TDvalue == 0) {
                    TDvalue = distributionUnifTD(generator);
                }
                arrivalTime.push_back(TDvalue);
                std::cout << " mean generation time " << arrivalTime[i] << std::endl;
            }

            break;
        }
        case (4): {
            double maxUnifValueTS = atof(argv[10]);
            std::uniform_real_distribution<> distributionUnifTS(0, maxUnifValueTS);
            double constValueTD = atof(argv[11]);


            for (int i = 0; i < nvertici; i++) {
                double TSvalue = 0;
                while (TSvalue == 0) {
                    TSvalue = distributionUnifTS(generator);
                }
                serviceTime.push_back(TSvalue);
                std::cout << " mean service time " << serviceTime[i] << std::endl;
            }

            for (int i = 0; i < nvertici; i++) {
                double TDvalue = constValueTD;
                arrivalTime.push_back(TDvalue);
                std::cout << " mean generation time " << arrivalTime[i] << std::endl;
            }

            break;
        }
    }



    if(THRESHOLDMode == 0) {
        THRESHOLD = 0; //tasks never migrate from the node that hosts them
    }
    else if(THRESHOLDMode == 1){
        THRESHOLD = 1; //tasks migrate when they overcome the threshold (method currently implemented)
    }
    else if(THRESHOLDMode == 2){
        THRESHOLD = 2; //tasks migrate according to the MWF policy (new method that has to be implemented)
    }

    if(migrationMode == 0) {
        migrationMODE = 0;
    }
    else if(migrationMode == 1){
        migrationMODE = 1;
    }

    if(failureMode == 0) {
        failureMODE = 0;
    }
    else if(failureMode == 1){
        failureMODE = 1;
    }


    nodeFail = nvertici*failedPerc/100;
    std::cout << "the number of nodes that will fail is: " <<nodeFail << std::endl;
    dec = 1;
    
    //adding mobile nodes to the topology
    coord_mobile_nodes = new Coordinates;
    mobile_nodes = new mobileNode[numMob];
    srand(time(NULL));
    for(int i = 0; i < numMob; i++){
        mobile_nodes[i].SetSpeed(speed);
        mobile_nodes[i].SetSpeedDirection((double)(rand() %360) * ((2*PI)/360));
        coord_mobile_nodes->SetCoordinates(Coord_rand(Border1), Coord_rand(Border1));
        mobile_nodes[i].SetPosition(coord_mobile_nodes);
        mobile_nodes[i].initcountMig();
    }
    
    //each mobile device generates a task
    //but they don't generate the first tasks at the same time
    double tGen = 0;
    for(int i = 0; i < numMob; i++){
        tGen =  (double)(rand()%2) + (double)rand() / (double)RAND_MAX;// time the mobile node will generate its first task
        ev = new Event(GENERATION, tGen, i, 0, 0); // the function generate() will generate the task of the mobile node i at the time tGen
        Lista.addEvent(ev); //the event 'generation' is added to the list of events
    }
    

    ev = new Event(CHECK, 0, 0, 0, 0); //event that evaluate the threshold and check if the threshold has been overcome
    Lista.addEvent(ev); //this event is added to the list of events
    std::uniform_real_distribution<> distributionTime(0, 5);
    double firstFailTime = distributionTime(generator);
    
    ev = new Event(FAILURE, firstFailTime, 0, 0, 0); //first time a node will fail
    Lista.addEvent(ev);//this event is added to the list of events
    
    ev = new Event(SIM_END, simulationTime, 0, 0, 0); //event that define the end of the simulation at the time simulationTime (the simulation could end before according to what we want)
    Lista.addEvent(ev);//this event is added to the list of events

    return option;
}

//===========================================================================
//= Function to initialize graph parameters
//===========================================================================
int manage::initialize_graph(int argc, char** argv){
    
    nvertici = atoi(argv[1]); //total number of vertices
    double id_node[nvertici],x_pos[nvertici], y_pos[nvertici];

    for(int i = 0; i< nvertici; i++) {
        for(int j = 0; j < nvertici; j++){
            Topology[i].initNeighbours();
        }
    }
    //array with the nodes still active
    for (int i = 0; i< nvertici; i++){
        arrayNvertici.push_back(i);
    }

    //node topology coordinates from file-----------------------------
    std::ifstream file_coord;
    file_coord.open(argv[2]);
    if (!file_coord)//test to see if file is open
    {
        std::cout << "Error opening coordinates file" << std::endl;
        return -1;
    }
    else
    {
        for (int i = 0 ; i<nvertici; i ++){
            file_coord >> id_node[i] >> x_pos[i] >> y_pos[i];
            std::cout << id_node[i] << " " << x_pos[i] << " " << y_pos[i] << std::endl;
        }
    }

    file_coord.close();


    //geographic topology creation ------------------------------------------------
    Grafo Topology2;
    for (int i = 0; i<nvertici; i++){
        Topology[i].SetX(x_pos[i]); //m
        Topology[i].SetY(y_pos[i]);
    }

    //edges from file-----------------------------
    FILE *file_edge = fopen(argv[3], "r" );
    if ( !file_edge )    {  //test to see if file is open
        std::cout << "Error opening edge file" << std::endl;
        return -1;
    }
    else igraph_read_graph_edgelist(&g1, file_edge,
                                    nvertici, 0); //undirected graph
    fclose(file_edge);


    //bandwidth from file-----------------------------
    std::ifstream file_band;
    file_band.open(argv[4]);
    if (!file_band)                  //test to see if file is open
    {
        std::cout << "Error opening bandwidth file" << std::endl;
        return -1;
    }

    else
    {
        double temp;
        while(file_band >> temp)
        {
            vec_band.push_back(temp);
        }
    }

    file_band.close();
    
    nedges = vec_band.size(); //number of edge in the topology

    //delays from file-----------------------------
    std::ifstream file_delays;
    file_delays.open(argv[5]);
    if (!file_delays)                  //test to see if file is open
    {
        std::cout << "Error opening delays file" << std::endl;
        return -1;
    }

    else
    {
        double temp;
        while(file_delays >> temp)
        {
            vec_delays.push_back(temp);
        }
    }

    file_delays.close();

    return 0;
}


//===========================================================================
//= Function to update the position of the mobile nodes
//===========================================================================
void manage::update_node_position(){

    for(int i = 0; i < numMob; i++) {
        mobile_nodes[i].UpdatePosition(CurrentTime);
        Coordinates *coord_mobile_nodes2;
        coord_mobile_nodes2 = mobile_nodes[i].GetPosition();
        //std::cout << "coord " <<coord_mobile_nodes2->GetCoordinateX() << " " <<  coord_mobile_nodes2->GetCoordinateY() << std::endl;
    }
}


//===========================================================================
//= Function to evaluate the delay from node i to node n
//===========================================================================
double manage::delayCalculation(int i, int n ){
    //std::cout << "error delayCalculation()" << std::endl;
    double timeMovedtask = 0.0;

    igraph_matrix_t res;
    igraph_bool_t connected;
    igraph_vs_t from;
    igraph_vs_t to;
    
    from = igraph_vss_1(i);
    to = igraph_vss_1(n);
    
    std::cout << "i: " << i << " n: " << n << std::endl;
    std::cout << "vertices: " << igraph_vcount(&g1) << " edges: " << igraph_ecount(&g1) << std::endl;

    igraph_matrix_init(&res, 0, 0);
    igraph_matrix_print(&res);
    igraph_is_connected(&g1, &connected, IGRAPH_STRONG);
    std::cout << connected << std::endl;

    igraph_shortest_paths(&g1, &res, from, to, IGRAPH_ALL);
    std::cout << "qui" << std::endl;

    if(MATRIX(res,0,0) == INFINITY ){
                std::cout << "the two nodes i and n are not connected " << std::endl;
        timeMovedtask = 10000;
    }
    if(MATRIX(res,0,0) > 0.5 && MATRIX(res,0,0) != INFINITY) {
                std::cout << " the two nodes i and n are connected, then tasks can migrate " << std::endl;
        std::vector<igraph_real_t> weights_delay; //vector of the delay of all the links of the topology

        //vector of the delay of all the links of the topolog
        for (int i = 0; i < nedges; i++) {
            weights_delay.push_back(vec_delays[i] * 100);
        }
        // delete from the vector weights_delay the delay of the link that don't belong anymore to the topology because nodes connected to those edges failed
        for(int j = 0; j < deletedEdges.size(); j ++ ) {
            weights_delay.erase(weights_delay.begin() + deletedEdges[j]);
        }
        igraph_real_t weights_delay2[weights_delay.size()];

        for (int i = 0; i < weights_delay.size(); i++) {
            weights_delay2[i] = weights_delay[i];

        }
        igraph_vector_t vertices1;
        igraph_vector_t edges1;
        igraph_vector_init(&vertices1, 0);
        igraph_vector_init(&edges1, 0);

        igraph_vector_view(&weights, weights_delay2, sizeof(weights_delay2) / sizeof(igraph_real_t));

        //evaluation of the shortest path between i and n
        igraph_get_shortest_path_dijkstra(&g1, &vertices1, &edges1, i, n, &weights, IGRAPH_ALL);

        //evaluation of the delay of the link of the shortest path between i and n
        for (int b = 0; b < igraph_vector_size(&edges1); b++) {
            int indice = VECTOR(edges1)[b];
            timeMovedtask = timeMovedtask + (vec_delays[indice] / (100 * 100));
        }

        igraph_vector_destroy(&edges1);
        igraph_vector_destroy(&vertices1);
    }

    return timeMovedtask;

}


//=================================================================================
//= Function that handles the migration of task (idTask) of the mobile node (nodeMob).
// The current position of the task in the current node's queue is posQueue
// This task moves from node (from) to node (to) an the time (timeMovedtask)
// When the task arrives at the new location, it is the last of the queue
//=================================================================================
void manage::migrate(double timeMovedtask, int from, int to, int posQueue, int nodeMob, int idTask){
    //std::cout << "error migrate()" << std::endl;

    Event* ev;
    mobile_nodes[nodeMob].setcountMig(idTask);
    Topology[from].setSizeQueue(Topology[from].getSizeQueue() - 1); //update size of the node's queue. The size decreases by one because the task migrates

    Topology[from].deleteMobileID(posQueue);
    Topology[from].deleteJobID(posQueue);

    ev = new Event(ARRIVAL, CurrentTime + timeMovedtask, nodeMob, idTask, to);// the task will arrive at the new node (to) at time CurrentTime + timeMovedtask
    Lista.addEvent(ev); //this event is added to the list of events
    Topology[to].setFrequency();
}

//=================================================================================
//= Function that handles the migration of task (idTask) of the mobile node (nodeMob)
// WITH PRIORITY
// The current position of the task in the current node's queue is posQueue
// This task moves from node (from) to node (to) an the time (timeMovedtask)
// When the task arrives at the new location, it is the first of the queue
//=================================================================================
void manage::migratePriority(double timeMovedtask, int from, int to, int posQueue, int nodeMob, int idTask){
    //std::cout << "error migratePriority()" << std::endl;

    Event* ev;
    mobile_nodes[nodeMob].setcountMig(idTask);

    Topology[from].setSizeQueue(Topology[from].getSizeQueue() - 1);

    Topology[from].deleteMobileID(posQueue);
    Topology[from].deleteJobID(posQueue);

    ev = new Event(ARRIVAL_PRIORITY, CurrentTime + timeMovedtask, nodeMob, idTask, to);// the task will arrive at the new node (to) at time CurrentTime + timeMovedtask with PRIORITY

    Lista.addEvent(ev); //add to the list of event this one
    Topology[to].setFrequency();
}

//===========================================================================
//= Function to check if node (from1) and node (to1) are connected
//===========================================================================
bool connectionCheck(int from1, int to1){
    //std::cout << "error connectionCheck()" << std::endl;
    int s;
    bool connessi;
    igraph_matrix_t res;
    igraph_bool_t connected;
    igraph_vs_t from;
    igraph_vs_t to;
    int prove = 0;
    int lengthPath;
    from = igraph_vss_1(from1);
    to = igraph_vss_1(to1);
    igraph_matrix_init(&res, 1, 1);
    //            igraph_matrix_print(&res);
    igraph_is_connected(&g1, &connected, IGRAPH_STRONG);
    igraph_shortest_paths(&g1, &res, from, to, IGRAPH_ALL);
    if(MATRIX(res,0,0) > 0.5 &&  MATRIX(res,0,0) != IGRAPH_INFINITY) { //if the selected vertex
        // is connected to the failing vertex, the program continues
        connessi = 1;
    }else{
        connessi = 0;
    }
    return connessi;
}


//=================================================================================
//= Function that evaluates the threshold and check if nodes overcome the threshold
//=================================================================================
void manage::check_status(int nodeMob, int option, int argc, char** argv){
    //std::cout << "error check_status()" << std::endl;

    // std::cout << " check " << std::endl;
    double threshold;


    double thresholdBuffer = 0;
    for (int i =0; i< nvertici; i++) {
        if (Topology[i].getState() == 1) {
            thresholdBuffer = thresholdBuffer + Topology[i].getSizeQueue();
        }
    }
    double threshold1 = thresholdBuffer/(nvertici - failedVertex);//nvertici;
    if(threshold1 > 2){
        threshold = abs(int(-threshold1));
    }
    else if(threshold1 <= 2){
        threshold = 2;
    }

    for (int i =0; i< nvertici; i++) {
        Topology[i].refreshInstantQueueSize();
        Topology[i].setInstantQueueSize(Topology[i].getSizeQueue());
    }


    double timeelab = 0;

    for (int i =0; i< nvertici; i++) {

        if(Topology[i].getSizeQueue() > threshold && Topology[i].getState() == 1) {            std::cout << "threshold " << threshold << "\t " << i  << "\t " << Topology[i].getSizeQueue() << "\t" << CurrentTime << std::endl;

            overcomedThreshold++;
            int size = Topology[i].getSizeQueue() - 1; // one task is being processed now
            //where the tasks move
            modeOffloading = argv[12];

            const std::string uniformMode = "uniform";
            const std::string variableMode = "variable";
            const std::string KSERVERMode = "kserver";

            int offloading;
            if(modeOffloading == uniformMode){
                offloading = 0;
            }
            else if(modeOffloading == variableMode){
                offloading = 1;
            }
            else if(modeOffloading == KSERVERMode){
                offloading = 2;
            }
            
            std::cout << "offloading mode " << modeOffloading  << std::endl;
  //kserverMODE = 1 -> harmonic on: when a node overcomes the threshold, its tasks migrate to the vertex chosen according to the harmonic policy

            switch (offloading) {
                //case 0 uniformMode: the task of the node that overcomes the thresold is migrated with uniform probability
                // to all nodes of the topology

                case 0: {
                    for (int x = size; x >= (1); x--) {
                        if (migrationMODE == 1) {
                            //case 0.1 migration cost on: the task of the node that overcomes the threshold migrates only if
                            // the migration cost of the new node is better than the migration cost of the current node
                            timeelab = migrationCostUniformON(nvertici, serviceTime,  x,  i,  timeelab);
                        }else if (migrationMODE == 0) {
                            //case 0.0 migration cost off: the task of the node that overcomes the threshold ALWAYS migrates
                            timeelab = migrationCostUniformOFF(nvertici, serviceTime,  x,  i,  timeelab);
                        }
                    }
                    break;
                }

                    //case 1 variableMode: the task of the node that overcomes the threshold migrates with higher probability
                    // to more powerful nodes of the topology
                case 1: {            //higher probability for the task with higher service time
                    //std::cout << " offloading caso 1: variabile" <<  size << std::endl;

                    for (int x = size; x >= (1); x--) {
                        //std::cout << " posizione " <<     x << " nel nodo  " <<  i << " generato dal nodo mobile " << Topology[i].getMobileID(x) << " task "  << Topology[i].getJobID(x) << std::endl;
                        //case 1.1 migration cost on: the task of the node that overcomes the threshold migrates only if
                        // the migration cost of the new nodes is better than the migration cost of the current node
                        if (migrationMODE == 1) {
                            timeelab = migrationCostVariableON(nvertici, serviceTime,  x,  i,  timeelab);
                        } else if (migrationMODE == 0) {
                            //case 1.0 migration cost off: the task of the node that overcomes the threshold ALWAYS migrates
                            timeelab = migrationCostVariableOFF(nvertici, serviceTime,  x,  i,  timeelab);
                        }
                    }
                    break;
                }
                    //case 2 kserverMODE: the task of the node that overcomes the threshold migrates according the kserver
                    //    problem. This case is used with failures
                case 2:
                {
                    for (int x = size; x >= (1); x--) {
                        //timeelab = migratekserverFailure(i, x, nodeMob, timeelab);
                        if (migrationMODE == 1) {
                            timeelab = migratekserverFailure(i, x, nodeMob, timeelab);
                        } else if (migrationMODE == 0) {
                            //case 1.0 migration cost off: the task of the node that overcomes the threshold ALWAYS migrates
                            timeelab = migrationCostUniformOFF(nvertici, serviceTime,  x,  i,  timeelab);
                        }
                    }
                    break;
                }
            }
            vecThreshold.push_back(threshold);
        }
    }

    Event *ev;
    ev = new Event(CHECK, CurrentTime+0.5, 0, 0, 0); //event that define the end of the simulation
    Lista.addEvent(ev);
}

//============================================================================================
//= Function that manages the migration when migrationMode == 1 and modeOffloading == variable.
//Migration cost on means that when a node's queue overcomes the threshold, its tasks ALWAYS
//migrate.
//modeOffloading == variable means that the vertex where tasks migrate is chosen among the
//more powerful active nodes (lower service time)
//This function
//1) triggers the migration if the node that is currently hosting the task has some active
//neighbors. If any neighbor is available, the task gets lost;
//2) returns the elaboration time taken by the node to elaborate the task
//============================================================================================
double manage::migrationCostVariableON(int nvertici, std::vector<double> serviceTime, int x, int i, double timeelab) {
    ////std::cout << "error migrationCostVariableON()" << std::endl;

    int n;
    if (Topology[i].getSizeNeighbours() > 1) {
        n = preferentialIDgen(i, nvertici, serviceTime);
        double transmissionCost_new = delayCalculation(i, n);

        if (transmissionCost_new != 10000) {
            timeelab = manageMigrationCost(serviceTime, transmissionCost_new, x, timeelab, n,
                                           i);
        }

    }

    else{
        lostTask++;
        int IDmob = Topology[i].getMobileID(x);
        std::cout << "lost " <<  i << "\t" <<   IDmob <<   "\t" <<   Topology[i].getJobID(x) << "\t" << CurrentTime <<std::endl;

        Topology[i].deleteMobileID(x);
        Topology[i].deleteJobID(x);
        mobile_nodes[IDmob].setLostTask();
        Topology[i].setSizeQueue(Topology[i].getSizeQueue() - 1);
    }
    return timeelab;
}



//============================================================================================
//= Function that manages the migration when migrationMode == 0 and modeOffloading == variable.
//Migration cost off means that when a node's queue overcomes the threshold, its tasks ALWAYS
//migrate.
//modeOffloading == variable means that the vertex where tasks migrate is chosen among the
//more powerful active nodes (lower service time)
//This function
//1) triggers the migration if the node that is currently hosting the task has some active
//neighbors. If any neighbor is available, the task gets lost;
//2) returns the elaboration time taken by the node to elaborate the task
//============================================================================================
double manage::migrationCostVariableOFF(int nvertici, std::vector<double> serviceTime, int x, int i, double timeelab) {
    //std::cout << "error migrationCostVariableOFF()" << std::endl;
    int n;
    if(Topology[i].getSizeNeighbours() > 1) {
        n = preferentialIDgen(i, nvertici, serviceTime);
        double transmissionCost_new = delayCalculation(i, n);
        if (transmissionCost_new != 10000) {
            migrate(transmissionCost_new + timeelab, i, n, x, Topology[i].getMobileID(x),
                    Topology[i].getJobID(x));

            migration++;
            timeelab = timeelab + 0.001;
        }
    }
    else{
        lostTask++;
        int IDmob = Topology[i].getMobileID(x);
        std::cout << "lost " <<  i << "\t" <<   IDmob <<   "\t" <<   Topology[i].getJobID(x) << "\t" << CurrentTime <<std::endl;

        Topology[i].deleteMobileID(x);
        Topology[i].deleteJobID(x);
        mobile_nodes[IDmob].setLostTask();
        Topology[i].setSizeQueue(Topology[i].getSizeQueue() - 1);
    }
    return timeelab;
}


//===========================================================================================
//= Function that manages the migration when migrationMode = 1 and modeOffloading == uniform.
//Migration cost on means that when a node's queue overcomes the threshold, its tasks migrate
//only if the migration cost of the new node is better than the migration cost of the current
//node.
//modeOffloading == uniform means that the vertex where tasks migrate is uniformly chosen
//among the active nodes
//This function
//1) triggers the migration if the node that is currently hosting the task has some active
//neighbors. If any neighbor is available, the task gets lost;
//2) returns the elaboration time taken by the node to elaborate the task
//===========================================================================================
double manage::migrationCostUniformON(int nvertici, std::vector<double> serviceTime, int x, int i, double timeelab) {
    std::cout << "error migrationCostUniformON()" << std::endl;
    int n;
    bool connessi;
    int prove = 0;
    std::uniform_int_distribution<int> distribution(0,nvertici-1);
    if(Topology[i].getSizeNeighbours() > 1) {
        do {
            n = distribution(generator);
        } while (n == i || Topology[n].getState() == 0 || Topology[i].getNeighbours(n) == 0);

        double transmissionCost_new = delayCalculation(i, n);

        if (transmissionCost_new != 10000) {
            timeelab = manageMigrationCost(serviceTime, transmissionCost_new, x, timeelab, n, i);
        }
    }
    else{
        lostTask++;
        int IDmob = Topology[i].getMobileID(x);
        std::cout << "lost " <<  i << "\t" <<   IDmob <<   "\t" <<   Topology[i].getJobID(x) << "\t" << CurrentTime <<std::endl;

        Topology[i].deleteMobileID(x);
        Topology[i].deleteJobID(x);
        mobile_nodes[IDmob].setLostTask();
        Topology[i].setSizeQueue(Topology[i].getSizeQueue() - 1);
    }
    return timeelab;
}


//===========================================================================================
//= Function that manages the migration when migrationMode = 0 and modeOffloading == uniform.
//Migration cost off means that when a node's queue overcomes the threshold, its tasks ALWAYS
//migrate.
//modeOffloading == uniform means that the vertex where tasks migrate is uniformly chosen
//among the active nodes
//This function
//1) triggers the migration if the node that is currently hosting the task has some active
//neighbors. If any neighbor is available, the task gets lost;
//2) returns the elaboration time taken by the node to elaborate the task
//===========================================================================================
double manage::migrationCostUniformOFF(int nvertici, std::vector<double> serviceTime, int x, int i, double timeelab) {
    std::cout << "error migrationCostUniformOFF()" << std::endl;
    int n;
    std::uniform_int_distribution<int> distribution(0,nvertici-1);
    if(Topology[i].getSizeNeighbours() > 1) {
        do {
            n = distribution(generator);
        } while (n == i || Topology[n].getState() == 0 || Topology[i].getNeighbours(n) == 0);


        double transmissionCost_new = delayCalculation(i, n);
        if (transmissionCost_new != 10000) {
            migrate(transmissionCost_new + timeelab, i, n, x, Topology[i].getMobileID(x), Topology[i].getJobID(x));

            migration++;
            timeelab = timeelab + 0.001;
        }
    }
    else{
        lostTask++;
        int IDmob = Topology[i].getMobileID(x);
        std::cout << "lost " <<  i << "\t" <<   IDmob <<   "\t" <<   Topology[i].getJobID(x) << "\t" << CurrentTime << std::endl;

        Topology[i].deleteMobileID(x);
        Topology[i].deleteJobID(x);
        mobile_nodes[IDmob].setLostTask();
        Topology[i].setSizeQueue(Topology[i].getSizeQueue() - 1);
    }
    return timeelab;
}

//=================================================================================
//= Function that evaluate the migration cost
//=================================================================================
double manage::manageMigrationCost(std::vector<double> serviceTime, double transmissionCost_new, int x, double timeelab, int n, int i){
    std::cout << "error manageMigrationCost()" << std::endl;


    double queueCost_new = serviceTime[n]/2 * Topology[n].getInstantQueueSize();
    double elaborationTime_new = timeelab;
    double migrationCost =
            transmissionCost_new + queueCost_new + elaborationTime_new;
    double queueCost_current = serviceTime[i]/2 * (x);
    double currentCost = queueCost_current;

    if (migrationCost < currentCost) {
        //std::cout << "It is worthwhile to migrate " << std::endl;
        migratePriority(transmissionCost_new + timeelab , i, n, x, Topology[i].getMobileID(x), Topology[i].getJobID(x));
        migration++;
        Topology[n].setInstantQueueSize(1);
        Topology[i].decrementInstantQueueSize(1);
        timeelab=timeelab + 0.001;
    } else {
        //std::cout << "It is not worthwhile to migrate " << std::endl;
        NOmigration++;
    }
    return timeelab;

}

//=================================================================================
//= Function to generate a new task
//=================================================================================
void manage::generate(int nodeMob, int option,  int argc, char** argv){
    //std::cout << "generate " << nodeMob << "\t" << CurrentTime << std::endl;
    std::cout << "error generate()" << std::endl;
    Event *ev;

    Coordinates *coord_mobile_nodes2;
    coord_mobile_nodes2 = mobile_nodes[nodeMob].GetPosition();
    int idClosestRouter = Topology[0].minDistance(Topology, coord_mobile_nodes2, nvertici);
    mobile_nodes[nodeMob].switchID(idClosestRouter);
    mobile_nodes[nodeMob].setJobID();
    mobile_nodes[nodeMob].setcountJob();
    mobile_nodes[nodeMob].initcountMig();

    int currentjobID = mobile_nodes[nodeMob].getcountJob();
    int nextjobID = mobile_nodes[nodeMob].getcountJob()+1;
    double wirelessDelay = Topology[idClosestRouter].Distance(coord_mobile_nodes2)*1000/ (SPEED_OF_LIGHT); // m/ms
    ev = new Event(ARRIVAL, CurrentTime + wirelessDelay, nodeMob, currentjobID, idClosestRouter);
    Lista.addEvent(ev);
    mobile_nodes[nodeMob].setGenerationTime(CurrentTime);

    int id = mobile_nodes[nodeMob].getID();

    generationMode = argv[19];
    const std::string uniformMod = "uniform";
    const std::string exponentialMod = "exponential";
    const std::string htMod = "HT";
    double AT = 0;
    if(generationMode == exponentialMod){
        AT = exponential(arrivalTime[id]);
    }
    else if(generationMode == uniformMod){
        AT = uniformDistr(arrivalTime[id]);
    }
    else if(generationMode == htMod){
        AT = htDistr(arrivalTime[id]);
    }


    ev = new Event(GENERATION, CurrentTime + AT, nodeMob, 0, 0);
    //std::cout << "the next task will be generated  at: " << CurrentTime + AT << std::endl;
    Lista.addEvent(ev);

    //check_status(0,option, argc, argv);


}

//====================================================================================
//= Function that manages the discrete eventts: GENERATION, ARRIVAL, ARRIVAL_PRIORITY,
//FAILURE, CHECK(threshold), SIM_END
//====================================================================================
int manage::clock_process(int option,  int argc, char** argv) {
    //std::cout << "error clock_process()" << std::endl;

    update_node_position();
    
    bool FAILED;
    FAILED = false;
    if(lostTask >= 1 ){
        FAILED = true;
    }
    Event *ev;
    ev = Lista.Get(); //get the first event of the list of events
    EventType tipo;
    tipo = ev->type;
    CurrentTime = ev->sched_time; //update the current time with the scheduled time at the beginning of the list of events.
    int mobNode = ev->mobID;
    int switchID = ev->switchID;
    int jobID = ev->jobID;

    //when a node fails. 500 is the total number of tasks. When the simulator processes 500 tasks, the simulation ends.
    if(totcompletedTask == (dec)*500/(nodeFail+1)){
        ev = new Event(FAILURE, CurrentTime, 0, 0, 0);
        Lista.addEvent(ev);
        dec++;
    }

    switch(tipo)
    {
        case GENERATION:
        {
            totGeneratedEvent++;
            generate(mobNode, option, argc, argv);
            break;
        }
        case DEPARTURE:
        {
            depart(mobNode, jobID, option, argc, argv);
            break;
        }
        case ARRIVAL:
        {
            if(Topology[switchID].getState() == 1){
                arrive(mobNode, jobID, switchID, option, argc, argv);
            }//else if(Topology[switchID].getState() == 0)
            //{
            //}
            break;

        }
        case ARRIVAL_PRIORITY:
        {
            arrivePriority(mobNode, jobID, switchID, option, argc, argv);
            break;
        }
        case FAILURE:
        {
            if(failureMODE == 1 && failedVertex < nodeFail) {
                failure(option, argc, argv);
            }
            break;
        }

        case CHECK:
        {
            if(THRESHOLD == 1) {
                check_status(mobNode, option, argc, argv);
            }
            else if(THRESHOLD == 2) {
                check_statusMWF(); //function to implement
            }
            break;
        }


        case SIM_END:
            //Lista.RemoveEvents();
            //std::cout << "End Simulation" << std::endl;
            //destroyVar();
            FAILED = true;
            break;

    }

    //return EndOfSimulation;
    return totcompletedTask;
    //return FAILED;
}




//=================================================================================
//= Function that manages the migration when the harmonic policy is used. When a
// node overcomes the threshold, all its tasks migrate to the node with lower c,
//where c is migration cost. The migration cost is evaluated for all active node
//If the node, that hosts the task that has to migrate, has any neighbor, the task
//gets lost
//=================================================================================
double manage::migratekserverFailure(int currentID, int queuePosition, int nodeMob, double timeelab){
    std::cout << "error migratekserverFailure()" << std::endl;

    int n;
    if(Topology[currentID].getSizeNeighbours() > 1) {
        do {
            n = kserverIDgen(currentID, nvertici, serviceTime, timeelab);
        } while (n == currentID || Topology[n].getState() == 0 || Topology[currentID].getNeighbours(n) == 0);

        double transmissionCost_new = delayCalculation(currentID, n);
        if (transmissionCost_new != 10000) {
            migrate(transmissionCost_new + timeelab, currentID, n, queuePosition,
                    Topology[currentID].getMobileID(queuePosition), Topology[currentID].getJobID(queuePosition));
            timeelab = timeelab + 0.001;
            migration++;
        }
    }
    else{
        lostTask++;
        int IDmob = Topology[currentID].getMobileID(queuePosition);
        std::cout << "lost " <<  currentID << "\t" <<   IDmob <<   "\t" <<   Topology[currentID].getJobID(queuePosition) << "\t" << CurrentTime <<std::endl;

        Topology[currentID].deleteMobileID(queuePosition);
        Topology[currentID].deleteJobID(queuePosition);
        mobile_nodes[IDmob].setLostTask();
        Topology[currentID].setSizeQueue(Topology[currentID].getSizeQueue() - 1);
    }
    return timeelab;
}



//====================================================================================
//= Function that manages when the node ends to process the task. and the task leaves
//the system.
//====================================================================================
void manage::depart(int nodeMob, int jobID, int option, int argc, char** argv){
    //std::cout << "error depart()" << std::endl;

    Event *ev;
    for (int i =0; i< nvertici; i++) {
        if (Topology[i].getDeparturetime() == CurrentTime && Topology[i].getState() == 1) {
            std::cout << "end " << i << "\t " << jobID << "\t " << nodeMob << "\t " << CurrentTime << std::endl;

            double taskLifeTime = CurrentTime - mobile_nodes[nodeMob].getGenerationTime(jobID);
            totLifeTaskTime.push_back(taskLifeTime);
            jobIDLifeTaskTime.push_back(jobID);

            totcompletedTask++;

            Topology[i].deleteMobileID(0);
            Topology[i].deleteJobID(0);

            Topology[i].setSizeQueue(Topology[i].getSizeQueue() -
                                     1); //decrease by 1 the number of tasks in the system because one was completed

            if (Topology[i].getSizeQueue() >= 1) {
                double DT = exponential(serviceTime[i]);

                Topology[i].setDeparturetime(CurrentTime + DT);
                ev = new Event(DEPARTURE, CurrentTime + DT, Topology[i].getMobileID(0), Topology[i].getJobID(0), 0);
                Lista.addEvent(ev);

            }

        }
    }
//    check_status(0,option, argc, argv );

}



//====================================================================================
//= Function that manages the arrive of a task in a node. When this function is called
//the task is put at the end of the queue. So, it will be the last to be served
//====================================================================================
void manage::arrive(int nodeMob, int jobID, int n, int option, int argc, char** argv){
    std::cout << "error arrive()" << std::endl;

    std::cout << "arrive " << n << " \t" << nodeMob << "\t " << jobID << "\t" << CurrentTime << std::endl;
    Topology[n].setarriveTime(CurrentTime);
    Event *ev;

    Topology[n].setSizeQueue(Topology[n].getSizeQueue() + 1);
    Topology[n].setMobileID(nodeMob);
    Topology[n].setJobID(jobID);
    Topology[n].initStateJob();

    //if there is just a job in the queue it will be served after DT seconds
    if(Topology[n].getSizeQueue() == 1)
    {
        double DT = exponential(serviceTime[n]);
        double temp = CurrentTime+DT;
        Topology[n].setDeparturetime(temp);
        mobile_nodes[nodeMob].setStartTime(CurrentTime);

        ev = new Event(DEPARTURE, CurrentTime+DT, nodeMob, jobID, 0);
        Lista.addEvent(ev);
    }

//    if(THRESHOLD == 1) {
       // check_status(0, option, argc, argv);
//    }

}



//====================================================================================
//= Function that manages the arrive of a task in a node. When this function is called
//the task is put after the last job insert with priority.
//====================================================================================
void manage::arrivePriority(int nodeMob, int jobID, int n, int option, int argc, char** argv){
    std::cout << "error arrivePriority()" << std::endl;



    Topology[n].setarriveTime(CurrentTime);
    Event *ev;
    int jobPriority = 0;

//Check the status of all tasks in the queue in order to find the position where this task will be insert
// It will be located after all the other job with priority
    if(Topology[n].getSizeQueue() == 0){
        jobPriority = 0;
    }
    else if(Topology[n].getSizeQueue() > 0 && Topology[n].getStateJob(0) == 0){
        jobPriority = 1;
        for (int y = 0; y < Topology[n].getSizeQueue(); y++) {
            if (Topology[n].getStateJob(y) == 1) {
                jobPriority++;
            }
        }
    }
    else {
        for (int y = 0; y < Topology[n].getSizeQueue(); y++) {
            if (Topology[n].getStateJob(y) == 1) {
                jobPriority++;
            }
        }
    }
    Topology[n].setSizeQueue(Topology[n].getSizeQueue() + 1);

    Topology[n].setMobileIDPriority(nodeMob, jobPriority);
    Topology[n].setJobIDPriority(jobID, jobPriority);
    //setto anche a questo job la priority settando 1 nella nuova posizione introdotta jobPriority
    Topology[n].setStateJob(jobPriority);


    //if there is just a job in the queue it will be served after DT seconds
    if(Topology[n].getSizeQueue() == 1)
    {
        double DT = exponential(serviceTime[n]);
        double temp = CurrentTime+DT;
        Topology[n].setDeparturetime(temp);
        mobile_nodes[nodeMob].setStartTime(CurrentTime);

        ev = new Event(DEPARTURE, CurrentTime+DT, nodeMob, jobID, 0);
        Lista.addEvent(ev);
    }

//    if(THRESHOLD == 1) {
       // check_status(0, option, argc, argv);
//    }

}


void manage::destroyGraph(){
    igraph_destroy(&g1);
}

//====================================================================================
//= Function that manages the failure of nodes
//====================================================================================
void manage::failure( int option, int argc, char** argv) {
    std::cout << "error failure()" << std::endl;

    //the id of the node that will fail is RANDOMLY chosen among the nodes that are still active
    const std::string uniformMode = "uniform";
    const std::string variableMode = "variable";
    const std::string KSERVERMode = "kserver";
    
    Event *ev;

    int n;
    int failurenodeId;
    modeOffloading = argv[12];

    //the id of the node that will fail is chosen among the more powerful vertices still active if kserver mode
    if (modeOffloading == uniformMode || modeOffloading == variableMode) {
        n = randId(arrayNvertici.size());
        failurenodeId = arrayNvertici[n];
    } else if (modeOffloading == KSERVERMode) {
        failurenodeId = powerfulIDgen(nvertici, serviceTime);
    }

    //arrayNvertici saves the vertices still active

    for (int q = 0; q < nvertici; q++){
        for(int p = 0; p < nvertici; p++) {
            if (Topology[q].getState() == 1 && Topology[p].getState() == 1) {
                int connessi = connectionCheck(p, q);
                Topology[q].setNeighbours(p, connessi);
            }
            else if (Topology[q].getState() == 0 || Topology[p].getState() == 0){
                Topology[q].setNeighbours(p, 0);
            }
        }
    }

    igraph_vector_t eids;
    igraph_vector_init(&eids, 0);
    igraph_es_t eids2;
    igraph_es_none(&eids2);

    //all the tasks of the failed node migrate
    int size = Topology[failurenodeId].getSizeQueue() - 1; // all tasks index from 0 to SizeQueue()-1
    std::cout << " failure " << failurenodeId << "  failedVertex " << failedVertex << " \t" << CurrentTime <<  std::endl;
    std::uniform_int_distribution<int> distribution(0, nvertici - 1);
    //update the number of failed vertices
    failedVertex++;
    double timeelab = 0;

    for (int x = size; x > (0); x--) {
        int s;
        igraph_matrix_t res;
        igraph_bool_t connected;
        igraph_vs_t from;
        igraph_vs_t to;
        int prove = 0;
        int lengthPath;
        do {
            do {
                //if modeOffloading == uniformMode, the vertex where the tasks migrate to is uniformly chosen among the active nodes
                if (modeOffloading == uniformMode) {
                    s = distribution(generator);
                    //std::cout << "proposed vertex " << s << std::endl;
                }
                    //if modeOffloading == variableMode, the vertex where the tasks migrate to is chosen among the more powerful active nodes
                else if (modeOffloading == variableMode) {
                    if(Topology[failurenodeId].getSizeNeighbours() > 1) {
                        s = preferentialIDgen(failurenodeId, nvertici, serviceTime);
                    }
                    else{
                        s = distribution(generator);
                    }
                }
                    //if modeOffloading == KSERVERMode, the vertex where the tasks migrate to is chosen according to kserver policy
                else if (modeOffloading == KSERVERMode) {
                    if(Topology[failurenodeId].getSizeNeighbours() > 1) {
                        s = kserverIDgen(failurenodeId, nvertici, serviceTime, timeelab);
                    }
                    else{
                        s = distribution(generator);
                    }
                }
                prove++;
            } while (failurenodeId == s || Topology[s].getState() == 0);
            from = igraph_vss_1(failurenodeId);
            to = igraph_vss_1(s);
            igraph_matrix_init(&res, 1, 1);
            igraph_is_connected(&g1, &connected, IGRAPH_STRONG);
            igraph_shortest_paths(&g1, &res, from, to, IGRAPH_ALL);

            if (MATRIX(res, 0, 0) > 0.5 && MATRIX(res, 0, 0) != IGRAPH_INFINITY) { //if the selected vertex
                // is connected to the failing vertex, the simulation continues
                break;
            }
        } while (prove < nvertici);
        if (MATRIX(res, 0, 0) > 0.5 && MATRIX(res, 0, 0) != IGRAPH_INFINITY) {
            //if the selected vertex is connected to the failing vertex, the task migrate
            double transmissionCost_new = delayCalculation(failurenodeId, s);
            migrate(transmissionCost_new + timeelab, failurenodeId, s, x, Topology[failurenodeId].getMobileID(x),
                    Topology[failurenodeId].getJobID(x));
            Topology[failurenodeId].setMobileID(size);
            migration++;
            timeelab = timeelab + 0.001;
        } else {
            //if the selected vertex is NOT connected to the failing vertex, the task is lost
            lostTask++;

            int IDmob = Topology[failurenodeId].getMobileID(x);
            std::cout << "perso " <<  failurenodeId << "\t" <<   IDmob <<   "\t" <<   Topology[failurenodeId].getJobID(x) << "\t" << CurrentTime <<std::endl;

            Topology[failurenodeId].deleteMobileID(x);
            Topology[failurenodeId].deleteJobID(x);
            mobile_nodes[IDmob].setLostTask();
            Topology[failurenodeId].setSizeQueue(Topology[failurenodeId].getSizeQueue() -
                                                 1);
        }
        igraph_matrix_destroy(&res);
    }
    //remove the edges that connect the failing nodes to the other vertices
    igraph_incident(&g1, &eids, failurenodeId, IGRAPH_ALL);
    for (int i = 0; i < igraph_vector_size(&eids); i++) {
        deletedEdges.push_back(VECTOR(eids)[i]);
    }
    std::sort(deletedEdges.rbegin(), deletedEdges.rend());

    igraph_es_vector_copy(&eids2, &eids);
    igraph_delete_edges(&g1, eids2);

    //update arrayNvertici by deleting the failed node
    int IDtodelete = 0;
    for (int i = 0; i < arrayNvertici.size(); i++) {
        if (arrayNvertici[i] == failurenodeId) {
            IDtodelete = i;
        }
    }
    arrayNvertici.erase(arrayNvertici.begin() + (IDtodelete));

    //set the state of the failed node to 0 (0 = failed, 1 = active)
    Topology[failurenodeId].setState(0);

// set the vertices that each node can reach because it is connected to them
    for (int q = 0; q < nvertici; q++){
        for(int p = 0; p < nvertici; p++) {
            if (Topology[q].getState() == 1 && Topology[p].getState() == 1) {
                int connessi = connectionCheck(p, q);

                Topology[q].setNeighbours(p, connessi);
            }
            else if (Topology[q].getState() == 0 || Topology[p].getState() == 0){
                Topology[q].setNeighbours(p, 0);
            }
        }
    }
    //new version FAILURE nodes
//    double AT = exponential(1);
//    
//    ev = new Event(FAILURE, CurrentTime + AT, 0, 0, 0); //10, 0, 0); //event that define the end of the simulation
//    Lista.addEvent(ev);

}




void manage::statistics(){
    double c = totcompletedTask/simulationTime;
    double totLifeTimeTask = 0;
    for (int i = 0; i < totLifeTaskTime.size(); i++){
        totLifeTimeTask = totLifeTimeTask + totLifeTaskTime[i];
    }
    int jobIDtotLifeTimeTask = 0;
    std::vector<int> freq_id;
    for (int num = 0; num < 100; num++) {
        freq_id.push_back(0);
    }
    for (int i = 0; i < jobIDLifeTaskTime.size(); i++){

        jobIDtotLifeTimeTask = jobIDtotLifeTimeTask + jobIDLifeTaskTime[i];
        for (int num = 0; num < 100; num++) {
            if (jobIDLifeTaskTime[i] == num){
                freq_id[num] = freq_id[num] + 1;
            }
        }
    }


    double thresholdAvg = 0;
    for (int i = 0; i < vecThreshold.size(); i++){
        thresholdAvg = thresholdAvg + vecThreshold[i];
    }
    thresholdAvg = thresholdAvg/vecThreshold.size();


    std::cout << "Simulation: " << CurrentTime << std::endl;
    std::cout << "totLifeTaskTime size: " <<totLifeTaskTime.size()<< std::endl;
    std::cout << "Events tot: " << totGeneratedEvent << std::endl;
    std::cout << "Time completed tasks: " << totLifeTimeTask << std::endl;
    std::cout << "average lifetime of a task: " << totLifeTimeTask/500 << std::endl;

    std::cout << "Completed tasks: " << totcompletedTask << std::endl;
    std::cout << "Throughput rate: " << c << std::endl;
    std::cout << "Server utilization rate: " << (totcompletedTask/simulationTime)*100 << std::endl;
    std::cout << "lost tasks " << lostTask << std::endl;
    std::cout << "migrated tasks " << migration << std::endl;
    std::cout << "NOmigrated tasks " << NOmigration << std::endl;
    std::cout << "threshold overcome" <<overcomedThreshold << std::endl;

    std::cout << "average threshold" << thresholdAvg << std::endl;


    for(int i = 0; i < nvertici; i ++){
        std::cout << "Freq: " << i << "\t" << Topology[i].getFrequency() << std::endl;
    }


    std::vector<std::vector<double>> arrayArriveTime;
    for(int i = 0; i < nvertici; i++) {
        std::vector<double> temp; // create an array, don't work directly on buff yet.
        for (int j = 1; j < Topology[i].getarriveTimeSize(); j++) {
            temp.push_back(Topology[i].getarriveTime(j) - Topology[i].getarriveTime(j-1));
        }
        arrayArriveTime.push_back(temp); // Store the array in the buffer
    }


    std::vector<double> averageArriveTime;
    for(int i = 0; i < nvertici; i ++) {
        averageArriveTime.push_back(0);
    }
    for(int i = 0; i < nvertici; i ++){
        for(int j = 0; j < Topology[i].getarriveTimeSize()-1; j ++) {
            averageArriveTime[i] = averageArriveTime[i] + arrayArriveTime[i][j];
        }
        if(averageArriveTime[i] != 0){
            std::cout << "Avg arrival: " << i << " \t " << averageArriveTime[i]/Topology[i].getarriveTimeSize() << std::endl;
        }
        else{
            std::cout << "Avg arrival: " << i << " \t " << averageArriveTime[i] << std::endl;
        }

    }

    std::cout << "end" << std::endl;
}

//===========================================================================
//= New function that implements MWF policy
//===========================================================================
void manage::check_statusMWF()
{
}
