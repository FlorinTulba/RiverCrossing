/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Requires Boost installation (www.boost.org).

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

#include "scenario.h"
#include "durationExt.h"
#include "transferredLoadExt.h"

#include <cfloat>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace boost::property_tree;

namespace {

/// @throw domain_error mentioning the set of keys to choose only one from
void duplicateKeyExc(const vector<string> &keys) {
  ostringstream oss;
  copy(CBOUNDS(keys), ostream_iterator<string>(oss, ", "));
  oss<<"\b\b ";
  throw domain_error("There must appear only one from the keys: "s + oss.str());
}

/**
@param pt the tree node that should contain one of the keyNames
@param keyNames the set of keys
@param firstFoundKeyName the returned key from the set
@return true if there is exactly one such key; false when no such key
@throw domain_error when the tree node contains more keys from the set
*/
bool onlyOneExpected(const ptree &pt, const vector<string> &keyNames,
    string &firstFoundKeyName) {
  firstFoundKeyName = "";
  size_t keysCount = 0ULL;
  for(const string &keyName : keyNames) {
    if(keysCount == 0ULL) {
      keysCount = pt.count(keyName);
      if(keysCount > 0ULL)
        firstFoundKeyName = keyName;
    } else
      keysCount += pt.count(keyName);

    if(keysCount >= 2ULL)
      duplicateKeyExc(keyNames);
  }

  return keysCount == 1ULL;
}

using namespace rc;
using namespace rc::ent;
using namespace rc::cond;

/**
Some scenarios mention how many entities can be simultaneously on the raft / bridge.
For the other scenarios is helpful to deduce an upper bound for this
transfer capacity.
When the capacity needs to be determined, it must be between
    2 and the count of all entities - 1,
that is the raft / bridge must hold >= 2 entities and there must remain
at least 1 entity on the other bank, to have a scenario that is not trivial.
*/
class TransferCapacityManager {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  shared_ptr<const AllEntities> entities; ///< all entities of the scenario

  /// Provided / deduced transfer capacity (&ScenarioDetails::capacity)
  unsigned &capacity;

public:
  TransferCapacityManager(const shared_ptr<const AllEntities> &entities_,
                          unsigned &capacity_) : // &ScenarioDetails::capacity
      entities(CP(entities_)), capacity(capacity_) {
    if(entities_->count() < 3ULL)
      throw domain_error(string(__func__) +
        " - there have to be at least 3 entities!");

    // Not all entities are allowed to cross the river simultaneously
    // (just to keep the problem interesting)
    capacity = (unsigned)CP(entities_)->count() - 1U;
  }

  /// The scenario specifies the transfer capacity
  void providedCapacity(unsigned capacity_) {
    if(capacity_ >= (unsigned)entities->count() || capacity_ < 2U)
      throw domain_error(string(__func__) +
        " - RaftCapacity / BridgeCapacity should be "
          "at least 2 and less than the number of entities!");
    if(capacity_ < capacity)
      capacity = capacity_;
  }

  /**
  The scenario specifies the raft / bridge max load, which might help
  for deducing the capacity.
  */
  void setMaxLoad(double maxLoad) {
    // Count lightest entities among which 1 might row whose total weight <= maxLoad
    const map<double, set<unsigned>> &idsByWeights = entities->idsByWeights();
    unsigned lightestEntWhoMightRow = UINT_MAX;
    double totalWeight = 0.;
    for(const auto &idsOfWeight : idsByWeights) {
      for(unsigned id : idsOfWeight.second)
        if( ! ! (*entities)[id]->canRow()) { // !!(tribool) => true or indeterminate
          lightestEntWhoMightRow = id;
          totalWeight = idsOfWeight.first;
          break;
        }
      if(lightestEntWhoMightRow != UINT_MAX)
        break;
    }
    assert(lightestEntWhoMightRow != UINT_MAX);
    assert(totalWeight != 0.);

    unsigned cap = 1U;
    for(const auto &idsOfWeight : idsByWeights) {
      const double weight = idsOfWeight.first;
      const set<unsigned> &sameWeightIds = idsOfWeight.second;
      size_t availableCount = sameWeightIds.size();
      if(sameWeightIds.find(lightestEntWhoMightRow) != cend(sameWeightIds))
        --availableCount;
      const size_t newCount =
        min(availableCount, (size_t)floor((maxLoad - totalWeight) / weight));
      if(0ULL != newCount) {
        cap += newCount;
        totalWeight += newCount * weight;
      }
      if(newCount < availableCount)
        break;
    }

    if(cap < 2U)
      throw domain_error(string(__func__) +
        " - Based on the entities' weights and the RaftMaxLoad/BridgeMaxLoad, "
        "the raft can hold at most one of them at a time! "
        "Please ensure the raft holds at least 2 entities!");

    if(cap >= (unsigned)entities->count())
      throw domain_error(string(__func__) +
        " - Based on the entities' weights and the RaftMaxLoad/BridgeMaxLoad, "
        "the raft can hold all of them at a time! "
        "This constraint cannot be counted as a valid scenario condition!");

    if(cap < capacity)
      capacity = cap;
  }

  /**
  The scenario specifies several constraints about the (dis)allowed raft/bridge
  configurations, which might help for deducing the capacity.
  */
  void setTransferConstraints(const TransferConstraints &transferConstraints) {
    const unsigned cap = transferConstraints.minRequiredCapacity();

    if(cap < 2U)
      throw domain_error(string(__func__) +
        " - Based on the "
        "[Dis]AllowedRaftConfigurations / [Dis]AllowedBridgeConfigurations, "
        "the raft can hold at most one entity at a time! "
        "Please ensure the raft holds at least 2 entities!");

    if(cap < capacity)
      capacity = cap;
  }

  const unsigned& getCapacity() const {return capacity;} ///< provided / deduced capacity
};

} // anonymous namespace

namespace rc {

Scenario::Scenario(istream &&scenarioStream, bool solveNow/* = false*/) {
	ptree pt, descrTree, entTree, crossingConstraintsTree,
		banksConstraintsTree, otherConstraintsTree;
	const ptree empty;
	try {
		read_json(scenarioStream, pt);
	} catch(const json_parser_error &ex) {
		throw domain_error(string(__func__) +
			" - Couldn't parse json file! Reason: "s + ex.what());
	}

	// Extract mandatory sections
	try {
		descrTree = pt.get_child("ScenarioDescription");
		entTree = pt.get_child("Entities");
		crossingConstraintsTree = pt.get_child("CrossingConstraints");
	} catch(const ptree_bad_path &ex) {
		throw domain_error(string(__func__) +
			" - Missing mandatory section! "s + ex.what());
	}

	// Store ScenarioDescription
	if(descrTree.count("") == 0ULL)
		throw domain_error(string(__func__) +
			" - The scenario description should be an array of 1 or more strings!");
	ostringstream oss;
	for(const auto &descrLine : descrTree)
		if(descrLine.first.empty())
			oss<<descrLine.second.data()<<endl;
		else
			throw domain_error(string(__func__) +
				" - The scenario description should be an array of 1 or more strings!");
	descr = oss.str();

	unsigned &capacity = details.capacity;
  shared_ptr<const AllEntities> &entities = details.entities;
	entities = make_shared<const AllEntities>(entTree);
	assert(entities && entities->count() > 0ULL);
	shared_ptr<const IEntity> firstEntity = (*entities)[*cbegin(entities->ids())];
	TransferCapacityManager capManager(entities, capacity);

	banksConstraintsTree = pt.get_child("BanksConstraints", empty);
	otherConstraintsTree = pt.get_child("OtherConstraints", empty);

	unsigned uniqueConstraints = 0U;
	string key;
	static const vector<string> capacitySynonyms{"RaftCapacity", "BridgeCapacity"};
	if(onlyOneExpected(crossingConstraintsTree, capacitySynonyms, key)) {
		try {
			// get<unsigned> fails to signal negative values
			const int readCapacity = crossingConstraintsTree.get<int>(key);
			if(readCapacity < 0)
				throw domain_error(string(__func__) +
					" - RaftCapacity / BridgeCapacity should be non-negative!");

      capManager.providedCapacity((unsigned)readCapacity);
		} catch(const ptree_bad_data &ex) {
			throw domain_error(string(__func__) +
				" - Bad type for the raft capacity! "s + ex.what());
		}

		++uniqueConstraints;
	}

	double &maxLoad = details.maxLoad;
	static const vector<string> maxLoadSynonyms{"RaftMaxLoad", "BridgeMaxLoad"};
	if(onlyOneExpected(crossingConstraintsTree, maxLoadSynonyms, key)) {
		try {
			maxLoad = crossingConstraintsTree.get<double>(key);
		} catch(const ptree_bad_data &ex) {
			throw domain_error(string(__func__) +
				" - Bad type for the raft max load! "s + ex.what());
		}

		if(maxLoad <= 0.)
			throw domain_error(string(__func__) +
				" - The raft max load cannot be negative or zero! "s);

		// If 1st entity has no specified weight, then none have.
		// However max load requires those properties
		if(firstEntity->weight() == 0.)
			throw domain_error(string(__func__) +
				" - Please specify strictly positive weights for all entities "
				"when using the `"s + key + "` constraint!"s);

    capManager.setMaxLoad(maxLoad);

		++uniqueConstraints;
	}

	shared_ptr<const IValues<double>> &allowedLoads = details.allowedLoads;
	static const vector<string> allowedLoadsSynonyms{"AllowedRaftLoads",
		"AllowedBridgeLoads"};
	if(onlyOneExpected(crossingConstraintsTree, allowedLoadsSynonyms, key)) {
		allowedLoads = move(grammar::parseAllowedLoadsExpr(
			crossingConstraintsTree.get<string>(key)));
		if( ! allowedLoads)
			throw domain_error(string(__func__) +
				" - AllowedRaftLoads parsing error! See the cause above."s);

		// If 1st entity has no specified weight, then none have.
		// However max load requires does properties
		if(firstEntity->weight() == 0.)
			throw domain_error(string(__func__) +
				" - Please specify strictly positive weights for all entities "
				"when using the `"s + key + "` constraint!"s);

		++uniqueConstraints;
	}

	// Keep this after capacitySynonyms, maxLoadSynonyms and allowedLoadsSynonyms
	const std::shared_ptr<const cond::ITransferConstraintsExt>
    transferConstraintsExt = details.createTransferConstraintsExt();

	static const vector<string> trConfigSpecifiers{"AllowedRaftConfigurations",
		"AllowedBridgeConfigurations", "DisallowedRaftConfigurations",
		"DisallowedBridgeConfigurations"};
	if(onlyOneExpected(crossingConstraintsTree, trConfigSpecifiers, key)) {
		boost::optional<grammar::ConstraintsVec> readConstraints =
			grammar::parseConfigurationsExpr(crossingConstraintsTree.get<string>(key));
		if( ! readConstraints)
			throw domain_error(string(__func__) +
				" - Constraints parsing error! See the cause above."s);

		// key starts either with 'A' or with 'D':
		// AllowedRaftConfigurations or AllowedBridgeConfigurations
		// DisallowedRaftConfigurations or DisallowedBridgeConfigurations
		const bool allowed = (key[0] == 'A');
		if( ! allowed) // ensure that DisallowedRaftConfigurations contains the empty set
			(*readConstraints).push_back(make_shared<const IdsConstraint>());

		try {
			details.transferConstraints = make_unique<const TransferConstraints>(
				move(*readConstraints), entities, capManager.getCapacity(), allowed,
        transferConstraintsExt);
		} catch(const logic_error &e) {
			throw domain_error(e.what());
		}

    capManager.setTransferConstraints(*details.transferConstraints);

		++uniqueConstraints;

	} else { // no [dis]allowed raft/bridge configurations constraint
    // Create a constraint checking only the capacity & maxLoad for the raft/bridge
    details.transferConstraints = make_unique<const TransferConstraints>(
            grammar::ConstraintsVec{}, entities,
            capManager.getCapacity(), false,
            transferConstraintsExt);
	}

	// Keep this after capacitySynonyms, maxLoadSynonyms and allowedLoadsSynonyms
	vector<ConfigurationsTransferDuration> &ctdItems = details.ctdItems;
  if(crossingConstraintsTree.count("CrossingDurationsOfConfigurations") > 0ULL) {
  	const ptree cdcTree =
  		crossingConstraintsTree.get_child("CrossingDurationsOfConfigurations");
	  if(cdcTree.count("") == 0ULL)
	    throw domain_error(string(__func__) +
	      " - The CrossingDurationsOfConfigurations section should be an array "
	      "of 1 or more such items!");

  	unordered_set<unsigned> durations;
    for(const auto &cdcPair : cdcTree) {
    	if( ! cdcPair.first.empty())
		    throw domain_error(string(__func__) +
		      " - The CrossingDurationsOfConfigurations section should be an array "
		      "of 1 or more such items!");
    	const string cdcStr = cdcPair.second.data();
    	boost::optional<grammar::ConfigurationsTransferDurationInitType> readCdc =
    		grammar::parseCrossingDurationForConfigurationsExpr(cdcStr);
			if( ! readCdc)
				throw domain_error(string(__func__) +
					" - CrossingDurationsOfConfigurations parsing error! See the cause above."s);
      ctdItems.emplace_back(move(*readCdc), entities,
                            capManager.getCapacity(),
                            transferConstraintsExt);
    	ConfigurationsTransferDuration &ctd = ctdItems.back();
    	if( ! durations.insert(ctd.duration()).second)
				throw domain_error(string(__func__) +
					" - Before configuration `"s + ctd.toString() +
					"` there was another one with the same transfer time. "
					"Please group them together instead!"s);
  	}

    ++uniqueConstraints;
  }

	if(uniqueConstraints == 0U)
		throw domain_error(string(__func__) +
			" - There must be at least one valid crossing constraint!");

  if( ! banksConstraintsTree.empty()) {
		static const vector<string> bankConfigSpecifiers{"AllowedBankConfigurations",
			"DisallowedBankConfigurations"};
		if(onlyOneExpected(banksConstraintsTree, bankConfigSpecifiers, key)) {
			boost::optional<grammar::ConstraintsVec> readConstraints =
				grammar::parseConfigurationsExpr(banksConstraintsTree.get<string>(key));
			if( ! readConstraints)
				throw domain_error(string(__func__) +
					" - Constraints parsing error! See the cause above."s);

			// key starts either with 'A' or with 'D':
			// AllowedBankConfigurations  OR  DisallowedBankConfigurations
			const bool allowed = (key[0] == 'A');

			// For AllowedBankConfigurations allow also start & final configurations
			if(allowed) {
				const vector<unsigned>
					&idsStartingFromLeftBank = entities->idsStartingFromLeftBank(),
					&idsStartingFromRightBank = entities->idsStartingFromRightBank();

				IdsConstraint
					*pInitiallyOnLeftBank = new IdsConstraint,
					*pInitiallyOnRightBank = new IdsConstraint;

				for(const unsigned id : idsStartingFromLeftBank)
					pInitiallyOnLeftBank->addMandatoryId(id);
				for(const unsigned id : idsStartingFromRightBank)
					pInitiallyOnRightBank->addMandatoryId(id);

				shared_ptr<const IConfigConstraint>
					initiallyOnLeftBank =
						shared_ptr<const IdsConstraint>(pInitiallyOnLeftBank),
					initiallyOnRightBank =
						shared_ptr<const IdsConstraint>(pInitiallyOnRightBank);

				(*readConstraints).push_back(initiallyOnLeftBank);
				(*readConstraints).push_back(initiallyOnRightBank);
			}

			try {
				details.banksConstraints = make_unique<const ConfigConstraints>(
					move(*readConstraints), entities,
					allowed);
			} catch(const logic_error &e) {
				throw domain_error(e.what());
			}
		}
  }

  unsigned &maxDuration = details.maxDuration;
  if( ! otherConstraintsTree.empty()) {
		static const vector<string> timeLimitSpecifiers{"TimeLimit"};
		if(onlyOneExpected(otherConstraintsTree, timeLimitSpecifiers, key)) {
			try {
				// get<unsigned> fails to signal negative values
				const int readMaxDuration = otherConstraintsTree.get<int>(key);
				if(readMaxDuration <= 0)
					throw domain_error(string(__func__) +
						" - TimeLimit should be > 0!");

				maxDuration = (unsigned)readMaxDuration;
			} catch(const ptree_bad_data &ex) {
				throw domain_error(string(__func__) +
					" - Bad type for the time limit! "s + ex.what());
			}

			// TimeLimit requires CrossingDurationsOfConfigurations constraints
			if(ctdItems.empty())
				throw domain_error(string(__func__) +
					" - Please specify a CrossingDurationsOfConfigurations section "
					"when using the `TimeLimit` constraint!"s);
		}
  }

  if(firstEntity->weight() > 0. // all entities have then specified weights
  		&& maxLoad == DBL_MAX && nullptr == allowedLoads)
		cout<<"[Notification] "<<__func__
			<<": Unnecessary weights of entities when not using none of the "
			"following constraints: "
			"{RaftMaxLoad/BridgeMaxLoad or AllowedRaftLoads/AllowedBridgeLoads}!"<<endl;

	if( ! ctdItems.empty() && maxDuration == UINT_MAX)
		cout<<"[Notification] "<<__func__
			<<": Unnecessary CrossingDurationsOfConfigurations "
			"when not using the TimeLimit constraint!"<<endl;

  if(solveNow)
  	solution();
}

shared_ptr<const cond::IContextValidator>
      ScenarioDetails::createTransferValidator() const {
  const shared_ptr<const cond::IContextValidator> &res =
      cond::DefContextValidator::INST();
  if(nullptr == allowedLoads)
    return res;

  return
    make_shared<const AllowedLoadsValidator>(allowedLoads, res,
      make_shared<const InitiallyNoPrevRaftLoadExcHandler>(allowedLoads));
}

shared_ptr<const cond::ITransferConstraintsExt>
      ScenarioDetails::createTransferConstraintsExt() const {
  const shared_ptr<const cond::ITransferConstraintsExt> &res =
      cond::DefTransferConstraintsExt::INST();
  if(maxLoad == DBL_MAX)
    return res;

  return
    make_shared<const MaxLoadTransferConstraintsExt>(maxLoad, res);
}

unique_ptr<ent::IMovingEntitiesExt>
      ScenarioDetails::createMovingEntitiesExt() const {
  unique_ptr<ent::IMovingEntitiesExt> res =
    make_unique<DefMovingEntitiesExt>();

  if(nullptr == allowedLoads && maxLoad == DBL_MAX)
    return res;

  return make_unique<TotalLoadExt>(entities, 0., std::move(res));
}

void Scenario::Results::update(const sol::IAttempt &unsuccessfulAttempt,
                               const ent::BankEntities &currentLeftBank,
                               size_t &bestMinDistToGoal) {
  const size_t attemptLen = unsuccessfulAttempt.length();
  if(attemptLen > longestInvestigatedPath)
    longestInvestigatedPath = attemptLen;

  const size_t crtDistToSol = unsuccessfulAttempt.distToSolution();
  if(bestMinDistToGoal > crtDistToSol) {
    bestMinDistToGoal = crtDistToSol;
    closestToTargetLeftBank = {currentLeftBank};

  } else if(bestMinDistToGoal == crtDistToSol) {
    closestToTargetLeftBank.push_back(currentLeftBank);
  }
}

const string& Scenario::description() const {
  return descr;
}

string Scenario::toString() const {
	ostringstream oss;
	oss<<details.toString();
	return oss.str();
}

string ScenarioDetails::toString() const {
	ostringstream oss;
	if(nullptr != entities)
		oss<<*entities<<endl;

	oss<<"CrossingConstraints: { ";
	if(capacity != UINT_MAX)
		oss<<"Capacity = "<<capacity<<"; ";
	if(maxLoad != DBL_MAX)
		oss<<"MaxLoad = "<<maxLoad<<"; ";
	if(nullptr != transferConstraints &&
        ! transferConstraints->empty())
		oss<<"TransferConstraints = "<<*transferConstraints<<"; ";
	if(nullptr != allowedLoads)
		oss<<"AllowedLoads = `"<<*allowedLoads<<"`; ";
	if( ! ctdItems.empty()) {
		oss<<"CrossingDurations: { ";
		copy(CBOUNDS(ctdItems),
			ostream_iterator<ConfigurationsTransferDuration>(oss, " ; "));
		oss<<"\b\b}; ";
	}
	oss<<"\b\b }";
	if(nullptr != banksConstraints &&
        ! banksConstraints->empty())
		oss<<endl<<"BanksConstraints = "<<*banksConstraints<<"; ";

	if(maxDuration != UINT_MAX) {
		oss<<endl<<"OtherConstraints: { ";
		oss<<"TimeLimit = "<<maxDuration<<"; ";
		oss<<"\b\b }";
	}

	return oss.str();
}

} // namespace rc

namespace std {

ostream& operator<<(ostream &os, const rc::Scenario &sc) {
	os<<sc.toString();
	return os;
}

ostream& operator<<(ostream &os, const rc::ScenarioDetails &sc) {
	os<<sc.toString();
	return os;
}

ostream& operator<<(ostream &os, const rc::Scenario::Results &o) {
  const shared_ptr<const rc::sol::IAttempt> &attempt = o.attempt;
  assert(attempt);
  if(attempt->isSolution()) {
    os<<"Found solution using "<<attempt->length()<<" steps:"<<endl<<*attempt;
  } else {
    os<<"Found no solution. "
      "Longest investigated path: "<<o.longestInvestigatedPath<<
      ". Investigated states: "<<o.investigatedStates<<
      ". Nearest states to the solution:"<<endl;
    for(const rc::ent::BankEntities &leftBank : o.closestToTargetLeftBank)
      os<<leftBank<<" - "<<~leftBank<<endl;
  }
  return os;
}

} // namespace std

#ifdef UNIT_TESTING

/*
	This include allows recompiling only the Unit tests project when updating the tests.
	It also keeps the count of total code units to recompile to a minimum value.
*/
#define SCENARIO_CPP
#include "../test/scenario.hpp"

#else // UNIT_TESTING not defined

#include <fstream>

#include <boost/filesystem/operations.hpp>

namespace fs = boost::filesystem;
using namespace fs;


namespace {

/**
  Takes the working directory and hopes
  it is (a subfolder of) RiverCrossing directory.

  @return RiverCrossing/Scenarios folder if found; otherwise an empty path
*/
const fs::path scenariosFolder() {
  fs::path dir = absolute(".");
  bool found = false;
  for(;;) {
    if(dir.filename().compare(fs::path("RiverCrossing")) == 0) {
      found = true;
      break;
    }

    if(dir.has_parent_path())
      dir = dir.parent_path();
    else
      break;
  }
  if( ! found || ! exists(dir /= "Scenarios"))
    return fs::path();

  return dir;
}

void tackleScenario(const fs::path &file, unsigned &solvedScenarios) try {
  cout<<"Handling scenario file: "<<file<<endl;
  rc::Scenario scenario(ifstream(file.string()));
  cout<<scenario<<endl<<endl;
  cout<<scenario.solution()<<endl<<endl<<endl;
  if(scenario.solution().attempt->isSolution())
    ++solvedScenarios;

} catch(const domain_error &ex) {
  cerr<<ex.what()<<endl;
}

} // anonymous namespace

int main(int /*argc*/, char* /*argv*/[]) {
	const fs::path dir = scenariosFolder();
	if(dir.empty()) {
		cerr<<"Couldn't locate the base folder of the project or the Scenarios folder! "
			"Please launch the program within (a subfolder of) RiverCrossing directory!"
			<<endl;
		return -1;
	}

	unsigned scenarios = 0U, solvedScenarios = 0U;

	// Traverse the Scenarios folder and solve the puzzle from each json file
	for(directory_iterator itEnd, it(dir); it != itEnd; ++it) {
		const fs::path file = it->path();
		if(file.extension().string() != ".json")
			continue;

    ++scenarios;
		tackleScenario(file, solvedScenarios);
	}

	cout<<"Solved "<<solvedScenarios<<'/'<<scenarios<<" scenarios."<<endl;

	return 0;
}

#endif // UNIT_TESTING not defined
