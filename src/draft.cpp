/*
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

**
Generates all k-combinations of the elements within first .. end.
If n = end - first, their count is: n!/(k!*(n-k)!)

@param first the first possible element
@param end the end iterator after the last possible element
@param k the group size
@param results vector of the resulted combinations
@param prefixComb the prefix for a k-combination (initially empty)

@throw logic_error if k is negative or more than end-first
*
template<class BidirIt, class CombVec>
void generate_combinations(BidirIt first, BidirIt end, ptrdiff_t k,
    std::vector<CombVec> &results, const CombVec &prefixComb = {}) {

  static_assert(std::is_same<
      typename std::iterator_traits<BidirIt>::value_type,
      typename CombVec::value_type>::value,
    "The iterator and the combinations vectors "
    "must point to values of the same type!");

  const ptrdiff_t n = std::distance(first, end);
  if(n < k || k < 0LL)
    throw std::logic_error(std::string(__func__) +
      " - Provided k must be non-negative and at most end-first!");

  if(k == 0LL) {
    results.push_back(prefixComb);
    return;
  }

  CombVec newPrefixComb(prefixComb);
  BidirIt it=first, itEnd = std::prev(end, --k);
  while(it != itEnd) {
    newPrefixComb.push_back(*it);
    generate_combinations(++it, end, k, results, newPrefixComb);
    newPrefixComb.pop_back();
  }
}
*/

/*
Input:

- Json Validation of bank & other constraints

- Condition for CanRow; ValRange for AllowedRaftLoads:
  to be validated, translated and processed by Boost::Proto.
  They need access to current state info (provides MoveIndex and PrevRaftContent)

- Configurations:
  - IdConfig to be transformed to regex
  - TypeConfig to be transformed to map<typeStrKey, pair<minInclUns, maxInclUns>>
  - For the DisAllowed form, just negate the match result

- Generating all possible raft/bridge configurations:
  a) Determine who can row (boolExpr.toString() != 'false')
  b) If unspecified, infer RaftCapacity
    - either as the largest allowed raft config
      (when AllowedRaft/BridgeConfigurations is specified)
    - or from the RaftMaxLoad (when specified):
      i) find the lightest entity (e0 weighing w0) able to row
        (boolExpr.toString() != 'false'))
      ii) count the lightest entities (apart from e0) whose total weight is
        at most (RaftMaxLoad - w0)
      iii) return 1 + the obtained count
    - otherwise set it to the count of entities
  c) generate combinations of all ids taken in groups of sizes
    progressing from 1 to RaftCapacity
*/

// Algorithm outline:

SymbolsTable st { CrossingIndex = 0, PreviousRaftLoad = NaN };
initState = State{..., ..., true, 0};
Solution sol{states=[], moves=[]};
examinedStates = [];

RaftConfigOption {
  cfg, validators []
}

RaftConfigOptions {
  ctor{
    generate all possible raft options when all entities are available
    ensuring canRow, capacity, maxLoad and [dis]allowedRaftCfgs are respected
    and adding validators for canRow and allowedLoads when a context is necessary
  }
  RaftConfigOption options[];
  raftCfg[] forBankCfg(bankCfg) {

  }
}

// f(canRow, capacity, maxLoad, allowedLoads, [dis]allowedRaftCfgs)
void raftConfigs(bankCfg, (&)options) {
  // There has to be at least 1 entity which can row for each reported option
  selfReliantEnts = bankCfg.idsOfRowCapableEnts(st);
  if(selfReliantEnts.empty()) return;

  if(option.intersects(selfReliantEnts))
    options.add(option);
}

/// @return true if a solution was found
bool findSol(Solution &sol, crtState, prevRaftCfg = NONE) {

  st.update(CrossingIndex++, PreviousRaftLoad = f(prevRaftCfg));
  sol.push(crtState, prevRaftCfg); examinedStates.push(crtState);

  raftConfigs(crtState.nextMoveFromLeft ? crtState.leftBank : crtState.rightBank,
      allPossibleRaftConfigs);
  for( raftCfg : allPossibleRaftConfigs ) {
    resultedState = crtState.next(raftCfg);
    if( ! resultedState.valid()
        || examinedStates.cover(resultedState)) continue; // check next raftCfg

    if(solution(resultedState)) {sol.push(resultedState, raftCfg); return true;}

    if(findSol(sol, resultedState, raftCfg)) return true;
  }

  // none of the moves develops towards a solution => backtracking
  sol.pop();
  st.update(CrossingIndex--, PreviousRaftLoad = sol.top().moves().top() OR NaN);

  return false;
}

if(findSol(sol, initState)) display(sol);
else sol = nullptr;
