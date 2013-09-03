/*
 * ccnSim is a scalable chunk-level simulator for Content Centric
 * Networks (CCN), that we developed in the context of ANR Connect
 * (http://www.anr-connect.org/)
 *
 * People:
 *    Giuseppe Rossini (lead developer, mailto giuseppe.rossini@enst.fr)
 *    Raffaele Chiocchetti (developer, mailto raffaele.chiocchetti@gmail.com)
 *    Dario Rossi (occasional debugger, mailto dario.rossi@enst.fr)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <cmath>
#include "statistics.h"
#include "core_layer.h"
#include "base_cache.h"
#include "content_distribution.h"
#include "lru_cache.h"

#include "ccnsim.h"
Register_Class(statistics);


using namespace std;


void statistics::initialize(){
    //Node property
    num_nodes 	= getAncestorPar("n");
    num_clients = getAncestorPar("num_clients");

    cTopology topo;
    
    //Statistics parameters
    ts          = par("ts"); //sampling time
    window      = par("window");
    time_steady = par("steady");
    partial_n 	= par("partial_n");
    if (partial_n == -1)
	partial_n = num_nodes;


    //Extracting clients
    clients = new client* [num_clients];
    vector<string> clients_vec(1,"modules.clients.client");
    topo.extractByNedTypeName(clients_vec);
    int k = 0;
    for (int i = 0;i<topo.getNumNodes();i++){
        int c = ((client *)topo.getNode(i)->getModule())->getNodeIndex();
	if ( find (content_distribution::clients, content_distribution::clients + num_clients, c) 
		!= content_distribution::clients + num_clients)  {
	    clients[k++] = (client *)topo.getNode(i)->getModule();
	}
    }

    //Extracting nodes (caches and cores)
    caches = new base_cache*[num_nodes];
    cores = new core_layer*[num_nodes];
    vector<string> nodes_vec(1,"modules.node.node");
    topo.extractByNedTypeName(nodes_vec);

    for (int i = 0;i<topo.getNumNodes();i++){
	caches[i] = (base_cache *) (topo.getNode(i)->getModule()->getModuleByRelativePath("cache"));
	cores [i] = (core_layer *) (topo.getNode(i)->getModule()->getModuleByRelativePath("core_layer"));
    }


    //Store samples for stabilization
    samples.resize(num_nodes);

    //Start checking for full
    scheduleAt(simTime() + ts, new cMessage("check_lfull",FULL_CHECK));

}



void statistics::handleMessage(cMessage *in){
    //Handle simulation timers

    int full = 0;
    int stables = 0;

    switch (in->getKind()){
        case FULL_CHECK:
            for (int i = 0; i < num_nodes;i++)
        	full += (int)caches[i]->full();

            if (full >= partial_n || simTime()>=10*3600){
        	cout<<"Caches filled at time "<<simTime()<<endl;
        	clear_stat();
        	scheduleAt(simTime() + ts, new cMessage("check_stability", STABLE_CHECK));
        	delete in;
            }else
        	scheduleAt(simTime() + ts, in);

            break;
//else if ( simTime()>=3*3600)
//		scheduleAt(simTime() + time_steady, new cMessage("end", END));


        case STABLE_CHECK:

            for (int i = 0;i<num_nodes;i++)
        	stables += (int) stable(i);

            if ( stables >= partial_n ){
        	scheduleAt(simTime() + time_steady, new cMessage("end", END));
        	clear_stat();
        	delete in;
            } else 
        	scheduleAt(simTime() + ts, in);
	    break;
        case END:
            delete in;
	    
            endSimulation();
    }

}

bool statistics::stable(int n){

    bool stable = false;
    double var = 0.0;
    double rate = caches[n]->hit * 1./ ( caches[n]->hit + caches[n]->miss );

    //Only hit rates matter, not alse the misses
    if (caches[n]->hit != 0 ){
        samples[n].push_back( rate );
    }else 
        samples[n].push_back(0);

    if ( fabs( samples[n].size() - window * 1./ts ) <= 0.001 ){ //variance each window seconds

	var =variance(samples[n]); 
        cout<<n<<"] variance = "<<var<<endl;
        if ( var <= 0.05){
            stabilization_time = simTime().dbl();
            stable = true;
        }
        samples[n].clear();

    }
    return stable;

}


void statistics::finish(){

    char name[30];

    uint32_t global_hit = 0;
    uint32_t global_miss = 0;
    uint32_t global_interests = 0;
    uint32_t global_data      = 0;

    double global_avg_distance = 0;
    simtime_t global_avg_time = 0;
    double global_tot_downloads = 0;

    for (int i = 0; i<num_nodes; i++){
	if (cores[i]->interests){//Check if the given node got involved within the interest/data process
	    global_hit  += caches[i]->hit;
	    global_miss += caches[i]->miss;
	    global_data += cores[i]->data;
	    global_interests += cores[i]->interests;
	}
    }

    
    //for (int k = 1; k<=num_nodes-2;k+=2){
    //    for (int i = k;i<=k+1;i++){
    //        lru_cache *lru = (lru_cache *) caches[i];
    //        if ( lru->actual_size==0 ) break;
    //        lru_pos *element = lru->lru;
    //        while (element != lru->mru){
    //    	level_union[k].insert(element->k);
    //    	level_same[k]++;
    //    	element = element->newer;
    //        }
    //        level_union[k].insert(element->k);
    //        level_same[k]++;
    //    }
    //}
    //
    //for (int i = 0;i<num_nodes;i++){
    //    lru_cache *lru = (lru_cache *) caches[i];
    //    int l = caches[i]->level;
    //    if ( l==0 || lru->actual_size==0 ) continue;

    //    lru_pos *element = lru->lru;
    //    while (element != lru->mru){
    //        level_union[l].insert(element->k);
    //        level_same[l]++;
    //        element = element->newer;
    //    }
    //    level_union[l].insert(element->k);
    //    level_same[l]++;
    //}


    //for (unordered_map<int,int>::iterator it = level_same.begin();it!=level_same.end();it++){
    //    sprintf (name, "diversity[%i]",it->first);
    //    recordScalar(name,level_union[it->first].size()*1./it->second);
    //    //cout<<it->first<<":"<<level_union[it->first].size()*1./it->second<<endl;
    //}

    //Print and store global statistics
    sprintf (name, "p_hit");
    recordScalar(name,global_hit * 1./(global_hit+global_miss));
    cout<<"p_hit/cache: "<<global_hit *1./(global_hit+global_miss)<<endl;

    sprintf ( name, "interests");
    recordScalar(name,global_interests * 1./num_nodes);

    sprintf ( name, "data" );
    recordScalar(name,global_data * 1./num_nodes);

    for (int i = 0;i<num_clients;i++){
	global_avg_distance += clients[i]->avg_distance;
	global_tot_downloads += clients[i]->tot_downloads;
	global_avg_time  += clients[i]->avg_time;
    }

    sprintf ( name, "hdistance");
    recordScalar(name,global_avg_distance * 1./num_clients);
    cout<<"Distance/client: "<<global_avg_distance * 1./num_clients<<endl;

    sprintf ( name, "avg_time");
    recordScalar(name,global_avg_time * 1./num_clients);
    cout<<"Time/client: "<<global_avg_time * 1./num_clients<<endl;

    sprintf ( name, "downloads");
    recordScalar(name,global_tot_downloads);
    cout<<"Downloads/client: "<<global_tot_downloads * 1./num_clients<<endl;

    
    //TODO global statistics per file
    //double hit_rate;
    // for (uint32_t f = 1; f <=content_distribution::perfile_bulk; f++){
    //     hit_rate = 0;
    //     if(hit_per_file[f]!=0)
    //         hit_rate = hit_per_file[f] / ( hit_per_file[f] +miss_per_file[f] );
    //     hit_per_fileV.recordWithTimestamp(f, hit_rate);
    //}
    
    
}

void statistics::clear_stat(){
    for (int i = 0;i<num_clients;i++)
	if (clients[i]->active)
	    clients[i]->clear_stat();

    for (int i = 0;i<num_nodes;i++)
	cores[i]->clear_stat();

    for (int i = 0;i<num_nodes;i++)
	caches[i]->clear_stat();
}
