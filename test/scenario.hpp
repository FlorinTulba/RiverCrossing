/******************************************************************************
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
 *****************************************************************************/

#if !defined CPP_SCENARIO || !defined UNIT_TESTING

#error \
    "Please include this file only within `scenario.cpp` \
after a `#define CPP_SCENARIO` and surrounding the include \
and the define by `#ifdef UNIT_TESTING`!"

#else  // for CPP_SCENARIO and UNIT_TESTING

#include "mathRelated.h"
#include "scenario.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/test/unit_test.hpp>

namespace fs = std::filesystem;

namespace {

void tackleScenario(const fs::path& file,
                    unsigned& solved,
                    unsigned& BFS_DFS_notableDiffs) try {
  using namespace std;

  rc::Scenario scenario{ifstream{file.string()}, false};
  // Defer solving to allow displaying its details first:
  cout << "Handling scenario file: " << file << '\n'
       << scenario << '\n'
       << endl;

  size_t bfsSolLen{}, dfsSolLen{};

  bfsSolLen = scenario.solution(true).attempt->length();
  if (bfsSolLen > 0ULL)
    ++solved;

  dfsSolLen = scenario.solution(false).attempt->length();
  if (dfsSolLen > 0ULL)
    ++solved;

  if (bfsSolLen != dfsSolLen) {
    ++BFS_DFS_notableDiffs;
    if (bfsSolLen > 0ULL && dfsSolLen > 0ULL)
      BOOST_CHECK(bfsSolLen <= dfsSolLen);
  }

} catch (const std::domain_error& ex) {
  std::cerr << ex.what() << std::endl;
  BOOST_CHECK(false);
}

}  // anonymous namespace

BOOST_AUTO_TEST_SUITE(scenario, *boost::unit_test::tolerance(rc::Eps))

BOOST_AUTO_TEST_CASE(invalidJSON) {
  BOOST_CHECK_THROW(rc::Scenario s{std::istringstream{R"(
    abc
    )"}},
                    std::domain_error);
}

BOOST_AUTO_TEST_CASE(missingMandatorySections) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"Entities" : [
        {"Id": 0,
        "Name": "Athlete",
        "CanRow": "true",
        "Weight": 90},
        {"Id": 1,
        "Name": "Coach",
        "CanRow": "true",
        "Weight": 80},
        {"Id": 2,
        "Name": "Fan",
        "CanRow": "true",
        "Weight": 70}
        ],

      "CrossingConstraints" : {
        "RaftMaxLoad": 100
        }
      }
    )"}},
                    domain_error);  // missing ScenarioDescription section

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["Nobody crossing the river"],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        }
      }
    )"}},
                    domain_error);  // missing Entities section

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ]
      }
    )"}},
                    domain_error);  // missing CrossingConstraints section
}

BOOST_AUTO_TEST_CASE(scenarioDescriptionValidation) {
  using namespace std;
  using rc::Scenario;

  // ScenarioDescription should be an array of string elements
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
        {"ScenarioDescription": 10,

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Name": "Coach"},
          {"Id": 2,
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "RaftCapacity": 2
          }
        }
    )"}},
                    domain_error);

  // ScenarioDescription should be an array of string elements
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
        {"ScenarioDescription": "abc",

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Name": "Coach"},
          {"Id": 2,
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "RaftCapacity": 2
          }
        }
    )"}},
                    domain_error);

  // ScenarioDescription should be an array of string elements
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
        {"ScenarioDescription": {"a":5},

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Name": "Coach"},
          {"Id": 2,
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "RaftCapacity": 2
          }
        }
    )"}},
                    domain_error);
}

BOOST_AUTO_TEST_CASE(emptyCrossingConstraintsSection) {
  BOOST_CHECK_THROW(rc::Scenario s{std::istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {}
      }
    )"}},
                    std::domain_error);
}

BOOST_AUTO_TEST_CASE(capacityValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeCapacity": "qwerty"
        }
      }
    )"}},
                    domain_error);  // non-number capacity

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People and coach crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 1.2
        }
      }
    )"}},
                    domain_error);  // non-integer capacity

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeCapacity": -1
        }
      }
    )"}},
                    domain_error);  // negative capacity

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 0
        }
      }
    )"}},
                    domain_error);  // provided capacity was 0 (less than 2)

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 1
        }
      }
    )"}},
                    domain_error);  // provided capacity was 1 (less than 2)

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeCapacity": 3
        }
      }
    )"}},
      domain_error);  // provided capacity was not less than entities' count

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2,
        "BridgeCapacity": 2
        }
      }
    )"}},
      domain_error);  // only one from {RaftCapacity, BridgeCapacity} can appear

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        }
      }
    )"}});

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeCapacity": 2
        }
      }
    )"}});
}

BOOST_AUTO_TEST_CASE(maxLoadValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeMaxLoad": "qwerty"
        }
      }
    )"}},
                    domain_error);  // non-number max load

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftMaxLoad": -100
        }
      }
    )"}},
                    domain_error);  // negative max load

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeMaxLoad": 0
        }
      }
    )"}},
                    domain_error);  // provided max load was 0

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["Peple crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftMaxLoad": 10,
        "BridgeMaxLoad": 20
        }
      }
    )"}},
      domain_error);  // only one from {RaftMaxLoad, BridgeMaxLoad} can appear

  // The entities must specify the Weight property when RaftMaxLoad appears
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftMaxLoad": 100
        }
      }
    )"}},
                    domain_error);

  // The entities must specify the Weight property when BridgeMaxLoad appears
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "BridgeMaxLoad": 100
        }
      }
    )"}},
                    domain_error);

  // Max load not enough to support at least 2 entities
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["Athlete crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftMaxLoad": 124
        }
      }
    )"}},
                    domain_error);

  // Max load way too large
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["Athlete crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftMaxLoad": 1000
        }
      }
    )"}},
                    domain_error);

  // Unnecessary weights
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["Athlete crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        }
      }
    )"}},
                    domain_error);

  BOOST_CHECK_NO_THROW({
    Scenario s{istringstream{R"(
        {"ScenarioDescription": ["People crossing the river"],

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Weight": 60,
          "Name": "Athlete"},
          {"Id": 1,
          "Weight": 65,
          "Name": "Coach"},
          {"Id": 2,
          "Weight": 80,
          "Name": "Fan"},
          {"Id": 3,
          "Weight": 55,
          "Name": "Host"}
          ],

        "CrossingConstraints" : {
          "RaftMaxLoad": 115
          }
        }
      )"}};
    BOOST_CHECK(s.details.capacity == 2U);
  });  // maxLoad ok for Athlete & Host

  BOOST_CHECK_NO_THROW({
    Scenario s{istringstream{R"(
        {"ScenarioDescription": ["Athlete crossing the river"],

        "Entities" : [
          {"Id": 0,
          "CanTackleBridgeCrossing": "true",
          "Weight": 60,
          "Name": "Athlete"},
          {"Id": 1,
          "Weight": 65,
          "Name": "Coach"},
          {"Id": 2,
          "Weight": 80,
          "Name": "Fan"},
          {"Id": 3,
          "Weight": 55,
          "Name": "Host"}
          ],

        "CrossingConstraints" : {
          "BridgeMaxLoad": 115
          }
        }
      )"}};
    BOOST_CHECK(s.details.capacity == 2U);
  });  // maxLoad ok for Athlete & Host
}

BOOST_AUTO_TEST_CASE(allowedLoadsValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedBridgeLoads": "qwerty"
        }
      }
    )"}},
                    domain_error);  // invalid value set expression

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftLoads": 100,
        "AllowedBridgeLoads": 150
        }
      }
    )"}},
      // only one from {AllowedRaftLoads, AllowedBridgeLoads} can appear
      domain_error);

  // Entities must specify the Weight property when AllowedRaftLoads appears
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftLoads": "50 .. 100"
        }
      }
    )"}},
                    domain_error);

  // Entities must specify the Weight property when AllowedBridgeLoads appears
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedBridgeLoads": "50 .. 100"
        }
      }
    )"}},
                    domain_error);

  // Unnecessary weights
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["Athlete crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        }
      }
    )"}},
                    domain_error);

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftLoads": "  14  , add(1,1) .. add(0, 5 mod 3), 9 .. 12  "
        }
      }
    )"}});

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Weight": 60,
        "Name": "Athlete"},
        {"Id": 1,
        "Weight": 65,
        "Name": "Coach"},
        {"Id": 2,
        "Weight": 80,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedBridgeLoads": " add(%b%, %c%),5 .. add(%a%, %b% mod %c%)  "
        }
      }
    )"}});
}

BOOST_AUTO_TEST_CASE(raftConfigurationsValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftConfigurations": "qwerty"
        }
      }
    )"}},
      domain_error);  // invalid configuration (unknown type 'qwerty')

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftConfigurations": "* *",
        "AllowedBridgeConfigurations": "* *"
        }
      }
    )"}},
                    domain_error);  // only one of the synonyms may appear

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftConfigurations": "* *",
        "DisallowedRaftConfigurations": "-"
        }
      }
    )"}},
                    domain_error);  // antonyms aren't allowed together

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftConfigurations": "* *",
        "DisallowedBridgeConfigurations": "-"
        }
      }
    )"}},
                    domain_error);  // antonyms aren't allowed together

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedBridgeConfigurations": "* *",
        "DisallowedRaftConfigurations": "-"
        }
      }
    )"}},
                    domain_error);  // antonyms aren't allowed together

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedBridgeConfigurations": "* *",
        "DisallowedBridgeConfigurations": "-"
        }
      }
    )"}},
                    domain_error);  // antonyms aren't allowed together

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "DisallowedRaftConfigurations": "*",
        "DisallowedBridgeConfigurations": "*"
        }
      }
    )"}},
                    domain_error);  // only one of the synonyms may appear

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftConfigurations": "*"
        }
      }
    )"}},
      domain_error);  // AllowedRaftConfigurations needs to allow >= 2 entities

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedRaftConfigurations": "* *"
        }
      }
    )"}});

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "AllowedBridgeConfigurations": "* *"
        }
      }
    )"}});

  try {
    Scenario s{istringstream{R"(
        {"ScenarioDescription": ["People crossing the river"],

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Name": "Athlete"},
          {"Id": 1,
          "Name": "Coach"},
          {"Id": 2,
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "DisallowedRaftConfigurations": "1"
          }
        }
      )"}};
    bool b{};
    BOOST_CHECK(b = (bool)s.details.transferConstraints);
    if (b) {
      rc::ent::MovingEntities me{s.details.entities};

      // explicitly disallowed
      BOOST_CHECK(!s.details.transferConstraints->check(me = {1}));

      // empty configuration is implicitly disallowed
      BOOST_CHECK(!s.details.transferConstraints->check(me = {}));
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  // empty bridge configuration is disallowed twice - implicitly and explicitly
  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanTackleBridgeCrossing": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "DisallowedBridgeConfigurations": "-"
        }
      }
    )"}});
}

BOOST_AUTO_TEST_CASE(crossingDurationsOfConfigurationsValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": 100
        }
      }
    )"}},
                    domain_error);  // expecting an array instead of 100

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": "abc"
        }
      }
    )"}},
                    domain_error);  // expecting an array instead of 'abc'

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": {"abc":5}
        }
      }
    )"}},
                    domain_error);  // expecting an array instead of {"abc":5}

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": []
        }
      }
    )"}},
                    domain_error);  // expecting a non-empty array

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [8]
        }
      }
    )"}},
      domain_error);  // wrong item in CrossingDurationsOfConfigurations

  // more items from CrossingDurationsOfConfigurations have same duration
  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Type": "female",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Type": "male",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Type": "male",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [
            "10 : 0",
            "10 : male"
          ]
        }
      }
    )"}},
                    domain_error);

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
        {"ScenarioDescription": ["People crossing the river"],

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Type": "female",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Type": "male",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Type": "male",
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "CrossingDurationsOfConfigurations": [
              "10 : 1- x female + 1+ x male",
              "15 : female"
            ]
          }
        }
      )"}},
                    domain_error);

  BOOST_CHECK_NO_THROW({
    Scenario s{istringstream{R"(
        {"ScenarioDescription": ["People crossing the river"],

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Type": "female",
          "Name": "Athlete"},
          {"Id": 1,
          "CanRow": "true",
          "Type": "male",
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Type": "male",
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "CrossingDurationsOfConfigurations": [
              "10 : 1- x female + 1+ x male",
              "15 : female"
            ]
          },

        "OtherConstraints" : {
          "TimeLimit": 100
          }
        }
      )"}};
    BOOST_CHECK(s.details.transferConstraints);  // default
    BOOST_CHECK(s.details.capacity == 2U);
  });  // count of entities - 1
}

BOOST_AUTO_TEST_CASE(bankConfigurationsValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(
      Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        },

      "BanksConstraints" : {
        "AllowedBankConfigurations": "qwerty"
        }
      }
    )"}},
      domain_error);  // invalid configuration (unknown type 'qwerty')

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "Name": "Coach"},
        {"Id": 2,
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        },

      "BanksConstraints" : {
        "AllowedBankConfigurations": "*",
        "DisallowedBankConfigurations": "-"
        }
      }
    )"}},
                    domain_error);  // antonyms aren't allowed together

  /*
  The scenario below won't throw.
  Although "AllowedBankConfigurations" mentions only "0 1 2? ; 2",
  the starting and final configurations are also allowed:
  "0 2 ; 1"
  */
  try {
    Scenario s{istringstream{R"(
        {"ScenarioDescription": [
          "People on opposite banks crossing the river"],

        "Entities" : [
          {"Id": 0,
          "CanRow": "true",
          "Weight": 60,
          "Name": "Athlete"},
          {"Id": 1,
          "Weight": 70,
          "StartsFromRightBank": true,
          "Name": "Coach"},
          {"Id": 2,
          "CanRow": "true",
          "Weight": 65,
          "Name": "Fan"}
          ],

        "CrossingConstraints" : {
          "AllowedRaftLoads": 130
          },

        "BanksConstraints" : {
          "AllowedBankConfigurations": "0 1 2? ; 2"
          }
        }
      )"}};
    bool b{};
    BOOST_CHECK(b = (bool)s.details.transferConstraints);  // default
    BOOST_CHECK(b = (bool)s.details.banksConstraints);
    if (b) {
      rc::ent::BankEntities be{s.details.entities};

      // explicitly allowed
      BOOST_CHECK(s.details.banksConstraints->check(be = {0, 1}));
      BOOST_CHECK(s.details.banksConstraints->check(be = {0, 1, 2}));
      BOOST_CHECK(s.details.banksConstraints->check(be = {2}));

      // next 2 configurations are implicitly allowed
      BOOST_CHECK(s.details.banksConstraints->check(be = {0, 2}));
      BOOST_CHECK(s.details.banksConstraints->check(be = {1}));
    }
  } catch (...) {
    BOOST_CHECK(false);  // Unexpected exception
  }

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "RaftCapacity": 2
        },

      "BanksConstraints" : {
        "DisallowedBankConfigurations": "0"
        }
      }
    )"}});
}

BOOST_AUTO_TEST_CASE(timeLimitValidation) {
  using namespace std;
  using rc::Scenario;

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [
            "10 : * ..."
          ]
        },

      "OtherConstraints" : {
        "TimeLimit": "abc"
        }
      }
    )"}},
                    domain_error);  // non-number time limit

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [
            "10 : * ..."
          ]
        },

      "OtherConstraints" : {
        "TimeLimit": 100.25
        }
      }
    )"}},
                    domain_error);  // non-integer time limit

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [
            "10 : * ..."
          ]
        },

      "OtherConstraints" : {
        "TimeLimit": -100
        }
      }
    )"}},
                    domain_error);  // negative time limit

  BOOST_CHECK_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [
            "10 : * ..."
          ]
        },

      "OtherConstraints" : {
        "TimeLimit": 0
        }
      }
    )"}},
                    domain_error);  // provided time limit is 0

  BOOST_CHECK_NO_THROW(Scenario s{istringstream{R"(
      {"ScenarioDescription": ["People crossing the river"],

      "Entities" : [
        {"Id": 0,
        "CanRow": "true",
        "Name": "Athlete"},
        {"Id": 1,
        "CanRow": "true",
        "Name": "Coach"},
        {"Id": 2,
        "CanRow": "true",
        "Name": "Fan"}
        ],

      "CrossingConstraints" : {
        "CrossingDurationsOfConfigurations": [
            "10 : * ..."
          ]
        },

      "OtherConstraints" : {
        "TimeLimit": 300
        }
      }
    )"}});
}

BOOST_AUTO_TEST_CASE(checkAllScenarioFiles) {
  using namespace std;

  fs::path dir{rc::projectFolder()};
  BOOST_REQUIRE(!dir.empty());
  BOOST_REQUIRE(exists(dir /= "Scenarios"));

  unsigned scenarios{}, solved{};

  // Count of the scenarios with worth-noting differences
  // between DFS and BFS strategies (solution length is different,
  // or one finds a solution and the other doesn't)
  unsigned BFS_DFS_notableDiffs{};

  // Traverse the Scenarios folder and solve the puzzle from each json file
  for (const auto& file : dir) {
    if (file.extension().string() != ".json")
      continue;

    ++scenarios;

    BOOST_TEST_CONTEXT("for puzzle: `" << file << '`') {
      tackleScenario(file, solved, BFS_DFS_notableDiffs);
    }
  }

  BOOST_CHECK(solved == 2 * scenarios);

  if (BFS_DFS_notableDiffs > 0U)
    cout << "There were " << BFS_DFS_notableDiffs
         << " scenarios with notable differences between BFS and DFS!" << endl;
}

BOOST_AUTO_TEST_SUITE_END()

#endif  // for CPP_SCENARIO and UNIT_TESTING
