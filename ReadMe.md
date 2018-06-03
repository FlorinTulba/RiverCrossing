### Describing and solving [River Crossing Puzzles](https://en.wikipedia.org/wiki/River_crossing_puzzle)

* * *

This type of puzzles requires moving several people / animals / goods (*entities*) from one bank of a river to the other bank using a raft / bridge.

Each problem comes with various constraints, like:

* which / how many entities are allowed to cross the river at once
* which / how many entities should remain on each bank
* a time limit for getting all entities to the other side, based on the speed of every required crossing of the river

Folder [Scenarios](./Scenarios/) contains the puzzles used for testing in *json* format.

Each puzzle file contains a description of the problem.

The entities should appear as follows:
```
"Entities" : [
	{"Id": <entity id - mandatory>,
	"Name": <entity name - mandatory>,
	"Type": <entity type - optional>,
	"CanRow": <boolean expression - optional; default: false; same as AllowedToCross>,
	"StartsFromRightBank": <boolean - optional; default: false>,
	"Weight": <entity weight - optional>},
	
	... - other entities
	]
```

Below are the possible specifiers for the constraints related to crossing the river. There must appear at least one such constraint.
```
"CrossingConstraints" : {
	"RaftCapacity": <max entities on the raft at once - optional ; Same as BridgeCapacity>,
	"RaftMaxLoad": <max weight supported by the raft - optional ; Same as BridgeMaxLoad>,
	"AllowedRaftConfigurations": <id-s of the entities that can use together the raft - optional ; Same as AllowedBridgeConfigurations>,
	"DisallowedRaftConfigurations": <id-s of the entities that cannot use alone the raft - optional ; Same as DisallowedBridgeConfigurations>,
	"CrossingDurationsOfConfigurations": [ several items like: "<crossing duration> : <configurations separated by ; > ] - optional information",
	"AllowedRaftLoads": <alternatives of total loads allowed on the raft during a given crossing - optional ; Same as AllowedBridgeLoads>
	}
```

These are the specifiers for the constraints for each river bank:
```
"BanksConstraints" : {
	"AllowedBankConfigurations": <id-s of the entities that can remain alone on any bank - optional>,
	"DisallowedBankConfigurations": <id-s of the entities that cannot remain alone on any bank - optional>
	}
```

Sometimes, some other constraints are needed:
```
"OtherConstraints" : {
	"TimeLimit": <max duration for transfering all entities to the other bank - optional>
	}
```

The scenarios use the following conventions:

- the first crossing starts always from the left bank
- a boolean expression is ```true```, ```false``` or an expression that evaluates to a boolean, optionally prefixed by ```if```
- ```%%``` is for dynamic expressions. **%CrossingIndex%** is the 1-based index of the current step / move / crossing; **%PreviousRaftLoad%** is the value of the raft load during the previous crossing (undefined for the first crossing)
- ```;``` delimits alternative expressions: 0 12 ; 3 14 5 which says that 0 12 and 3 14 5 are both valid configurations
- ```|``` delimits alternatives within an expression: 13 (24 | 5) which means that 13 24 and 13 5 are valid expressions
- ```...``` allows accepting any number of the remaining entities: 13 ... stands for 13 ; 13 0 ; 13 0 1 ; 13 0 1 2 ; 13 0 1 2 3 ; a.s.o.
- a ```!``` prefix denotes the lack of the prefixed entity: 22 4 !18 ... which means that 22 4 are expected and there might be other entities apart from 18
- a ```?``` suffix denotes an optional entity: 32 14? 48 which reads that 32 48 and 32 14 48 are valid expressions
- ```*``` allows any entity apart from the ones mentioned within the current expression: 10 2 * denotes 10 2 0 ; 10 2 1 ; 10 2 3 ; 10 2 4 ; a.s.o.
- ```-``` is used to unqualify all entities (the empty set)
- ```3+ x employee + 2 x manager + 10- x visitor + child``` means at least 3 entities of type employee, exactly 2 entities of type manager, at most 10 visitors and exactly 1 child
- ```v1..v2``` defines the v1-v2 range
- ```add(v1, v2)``` means v1 + v2
- ```v mod n``` is the remainder of v divided by n
- ```v [not] in {values}``` is true if v is [not] among values

The fictitious scenario from below covers the syntax used so far:
```
{"ScenarioDescription": [],
"Entities" : [
	{"Id": 0,
	"Name": "JohnDoe",
	"Type": "employee",
	"Weight": 82,
	"CanRow": "if (%CrossingIndex% mod 3) in {1, 2}",
	"StartsFromRightBank": true},

	{"Id": 1,
	"Name": "Jane",
	"Type": "manager",
	"Weight": 45},

	{"Id": 2,
	"Name": "Vincent",
	"CanRow": true,
	"Type": "visitor",
	"Weight": 75},

	... - several other entities with id-s starting from 3
	],

"CrossingConstraints" : {
	"RaftCapacity": 2,
	"RaftMaxLoad": 160,
	"AllowedRaftLoads": "add(%PreviousRaftLoad%, -10) .. add(%PreviousRaftLoad%, 10)",
	"CrossingDurationsOfConfigurations": [
		"10 : 0 1? ; 5",
		"20 : (2 | 3 | 4) (0 | 1)?",
		... - remaining possible configurations with their durations
		],

	!!! Use at most one of the AllowedRaftConfigurations and DisallowedRaftConfigurations
	!!! Below they appear both, just to exemplify more cases
	"AllowedRaftConfigurations": "* ; 2 x employee ; 2 x manager ; employee + visitor ; 0 1",
	"DisallowedRaftConfigurations": "0 (4 | 5)? ; 3 (1 | 2 | 6) ; 1 * ; 4 * ; 2 6"
	},

"BanksConstraints" : {
	!!! Use at most one of the AllowedBankConfigurations and DisallowedBankConfigurations
	!!! Below there appear more of them, just to exemplify more cases
	"AllowedBankConfigurations": "0 1 ; 0 (3 | 4 | 5) !1 ... ; 2 !3 * ...",
	"DisallowedBankConfigurations": "1 (0 | 2) ; 4 (3 | 5) ; 3 x employee + 2 x manager ; 2+ x employee + manager",
	"DisallowedBankConfigurations": "1+ x employee + 0 x manager + 1+ x visitor"
	},

"OtherConstraints" : {
	"TimeLimit": 1000
	}
}
```
Explanations of the example scenario:

- there are 3 types of entities: *employee*, *manager* and *visitor*
- the employee JohnDoe who weighs 82kg is initially on the other bank (the right one, so at the end he needs to be on the left bank). He can row, but won't row every third crossing (third, sixth, ninth ...): ```CanRow if (%CrossingIndex% mod 3) in {1, 2}```
- the manager Jane weighs 45kg and cannot row
- the visitor Vincent weighs 75kg and can row (without restrictions)
- the raft supports a maximum load of 160kg and carries only 2 passengers at once. Besides, any consecutive raft loads cannot vary with more than 10kg: ```AllowedRaftLoads add(%PreviousRaftLoad%, -10) .. add(%PreviousRaftLoad%, 10)```
- every person needs to reach their target bank in under 1000s
- it takes 10s for employee JohnDoe (id 0) to cross the river alone or together with the manager Jane (id 1). Entity with id 5 needs 10s as well for the traversal: ```CrossingDurationsOfConfigurations 10 : 0 1? ; 5```
- it takes 20s for the persons with the id-s 2, 3 or 4 to cross the river alone or together with either JohnDoe (id 0) or Jane (id 1): ```CrossingDurationsOfConfigurations 20 : (2 | 3 | 4) (0 | 1)?```
- the raft can be used by any person (who can row) or by 2 employees, or by 2 managers, or by an employee and a visitor or by the persons with id-s 0 and 1: ```AllowedRaftConfigurations * ; 2 x employee ; 2 x manager ; employee + visitor ; 0 1```
- the raft cannot be used by the persons with id-s 0 alone or together with 4 or 5, neither by 3 together with 1, 2 or 6. Furthermore, 1 and 4 must row alone on the raft and 2 and 6 cannot use the raft simultaneously: ```DisallowedRaftConfigurations 0 (4 | 5)? ; 3 (1 | 2 | 6) ; 1 * ; 4 * ; 2 6```
- at a given time, on any bank there can be together either people with id-s 0 and 1, or 0 with one of 3, 4 or 5, but without 1, plus any number of the remaining people. Finally, an acceptable configuration is also 2, without 3, plus at least one of the remaining persons: ```AllowedBankConfigurations 0 1 ; 0 (3 | 4 | 5) !1 ... ; 2 !3 * ...```
- do not leave alone on a bank following people: 1 with either 0 or 2; 4 with either 3 or 5 and neither more employees than managers: ```DisallowedBankConfigurations 1 (0 | 2) ; 4 (3 | 5) ; 3 x employee + 2 x manager ; 2+ x employee + manager```
- make sure visitors are not together with the employees in the absence of managers: ```DisallowedBankConfigurations 1+ x employee + 0 x manager + 1+ x visitor```

- - -

The program will signal scenarios that are either trivial or unfeasible. Examples of such scenarios:

- when all entities can board the raft at once and thus perform a single crossing
- when the raft can hold only a single entity. Such an entity will cross the river just to realize it must cross the river back to return the raft. Situations like that prevent any progress towards a solution
- when no entity from the starting bank can row

So, valid scenarios come with a raft that can hold at least 2 entities. The total number of entities must be larger than the raft capacity. Furthermore, some entities from the starting bank should be able to row.

The raft / bridge capacity might be deduced or reduced based on several different provided constraints, like the max load or the (dis)allowed raft / bridge configurations.

- - -

The project was compiled with g++ for C++14 under Cygwin platform, as reported by the unit tests:

```
Running 182 test cases...
Platform: Cygwin
Compiler: GNU C++ version 6.4.0
STL     : GNU libstdc++ version 20170704
Boost   : 1.67.0
Entering test module "RiverCrossing_tests"
.........................................
Leaving test module "RiverCrossing_tests"; testing time: 78ms

Test module "RiverCrossing_tests" has passed with:
  182 test cases out of 182 passed
  2059 assertions out of 2059 passed
```

So far, the results are simply displayed within the console. Here is the output for the classical [*Farmer - Wolf - Goat - Cabbage*](./Scenarios/wolfGoatCabbage.json) puzzle:

```
Handling scenario file: "Scenarios\wolfGoatCabbage.json"
Entities: [ Entity 0 {Name: `Farmer`, CanRow: `true`}, Entity 1 {Name: `Wolf`}, Entity 2 {Name: `Goat`}, Entity 3 {Name: `Cabbage`} ]
CrossingConstraints: { Capacity = 2 }
BanksConstraints = NOT{ [ Mandatory={2 extra_ids_count=1} Avoided={0} any_number_from_the_others ] };

Found solution using 7 steps:
Left bank: [ Farmer(0), Wolf(1), Goat(2), Cabbage(3) ] ; Right bank: [] ; Next move direction:  -->

        move   1:       >>>> [ Farmer(0), Goat(2) ] >>>>

Left bank: [ Wolf(1), Cabbage(3) ] ; Right bank: [ Farmer(0), Goat(2) ] ; Next move direction:  <--

        move   2:       <<<< [ Farmer(0) ] <<<<

Left bank: [ Farmer(0), Wolf(1), Cabbage(3) ] ; Right bank: [ Goat(2) ] ; Next move direction:  -->

        move   3:       >>>> [ Farmer(0), Cabbage(3) ] >>>>

Left bank: [ Wolf(1) ] ; Right bank: [ Farmer(0), Goat(2), Cabbage(3) ] ; Next move direction:  <--

        move   4:       <<<< [ Farmer(0), Goat(2) ] <<<<

Left bank: [ Farmer(0), Wolf(1), Goat(2) ] ; Right bank: [ Cabbage(3) ] ; Next move direction:  -->

        move   5:       >>>> [ Farmer(0), Wolf(1) ] >>>>

Left bank: [ Goat(2) ] ; Right bank: [ Farmer(0), Wolf(1), Cabbage(3) ] ; Next move direction:  <--

        move   6:       <<<< [ Farmer(0) ] <<<<

Left bank: [ Farmer(0), Goat(2) ] ; Right bank: [ Wolf(1), Cabbage(3) ] ; Next move direction:  -->

        move   7:       >>>> [ Farmer(0), Goat(2) ] >>>>

Left bank: [] ; Right bank: [ Farmer(0), Wolf(1), Goat(2), Cabbage(3) ]
```

The provided [Makefile](./Makefile) allows generating the binaries for the release / debug and for the unit tests. The executables can then be launched using the corresponding *run&lt;Configuration&gt;.sh* command.

- - -

The puzzles from the [Scenarios](./Scenarios/) folder are from a [River Crossing game](https://play.google.com/store/apps/details?id=com.androyal.rivercrossing.complete) from Google Play Store. The [bridge and torch problem](https://en.wikipedia.org/wiki/Bridge_and_torch_problem) is from Wikipedia.

* * *

&copy; 2018 Florin Tulba (florintulba@yahoo.com)
