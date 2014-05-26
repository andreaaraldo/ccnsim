/*
 * ccnSim is a scalable chunk-level simulator for Content Centric
 * Networks (CCN), that we developed in the context of ANR Connect
 * (http://www.anr-connect.org/)
 *
 * People:
 *    Giuseppe Rossini (lead developer, mailto giuseppe.rossini@enst.fr)
 *    Raffaele Chiocchetti (developer, mailto raffaele.chiocchetti@gmail.com)
 *    Dario Rossi (occasional debugger, mailto dario.rossi@enst.fr)
 *    Andrea Araldo (mailto andrea.araldo@gmail.com)
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
#ifndef COSTPROBPRODTAILPERF_POLICY_H_
#define COSTPROBPRODTAILPERF_POLICY_H_

//<aa>
#include "decision_policy.h"
#include "error_handling.h"
#include "costprob_policy.h"
#include "lru_cache.h"
#include "WeightedContentDistribution.h"
#include "costprobtail_policy.h"

// I verified that xi=1 is the best setting

class Costprobtailperf: public Costprobtail{
	public:
		Costprobtailperf(double average_decision_ratio_, base_cache* mycache_par):
			Costprobtail(average_decision_ratio_, mycache_par)
		{			
			// Do nothing
		};

		virtual double compute_content_weight(chunk_t id, double cost)
		{
			double popularity_estimation = 1./pow(id, alpha);
			return cost * popularity_estimation;
		}

		virtual bool decide_with_cache_not_full(chunk_t id, double cost){
			return true;
		}
};
//<//aa>
#endif

