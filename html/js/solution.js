// This file was generated and is not one of the sources of the project
// RiverCrossing (https://github.com/FlorinTulba/RiverCrossing).
// It contains the solution for a River-Crossing puzzle and is used
// during the visualization of that solution within a browser
var solution = {
    "ScenarioDescription": [
        "5 animals want to cross the river: a mouse, a chicken, a bear and 2 gorillas.",
        "Constraints:",
        "- all animals can row, but the raft can only hold the 2 small animals (mouse and chicken) or one big animal (a bear or a gorilla)",
        "- the mouse should not be left alone on either bank",
        "- the chicken should not be left alone with any gorilla"
    ],
    "Entities": [
        {
            "Id": "0",
            "Name": "Mouse",
            "CanRow": "true",
            "Image": {
                "Url": "..\/Scenarios\/images\/mouse.png",
                "Origin": "https:\/\/pngimg.com\/uploads\/rat_mouse\/rat_mouse_PNG23540.png"
            }
        },
        {
            "Id": "1",
            "Name": "Chicken",
            "CanRow": "true",
            "Image": {
                "Url": "..\/Scenarios\/images\/chicken.png",
                "Origin": "https:\/\/pngimg.com\/uploads\/chicken\/chicken_PNG2163.png"
            }
        },
        {
            "Id": "2",
            "Name": "Bear",
            "CanRow": "true",
            "Image": {
                "Url": "..\/Scenarios\/images\/bear.png",
                "Origin": "https:\/\/pngimg.com\/uploads\/bear\/bear_PNG1192.png"
            }
        },
        {
            "Id": "3",
            "Name": "Gorilla1",
            "CanRow": "true",
            "Image": {
                "Url": "..\/Scenarios\/images\/gorilla.png",
                "Origin": "https:\/\/pngimg.com\/uploads\/gorilla\/gorilla_PNG18713.png"
            }
        },
        {
            "Id": "4",
            "Name": "Gorilla2",
            "CanRow": "true",
            "Image": {
                "Url": "..\/Scenarios\/images\/gorilla.png"
            }
        }
    ],
    "Moves": [
        {
            "Idx": "0",
            "LeftBank": [
                "0",
                "1",
                "2",
                "3",
                "4"
            ],
            "RightBank": ""
        },
        {
            "Idx": "1",
            "Transferred": [
                "0",
                "1"
            ],
            "LeftBank": [
                "2",
                "3",
                "4"
            ],
            "RightBank": [
                "0",
                "1"
            ]
        },
        {
            "Idx": "2",
            "Transferred": [
                "0"
            ],
            "LeftBank": [
                "0",
                "2",
                "3",
                "4"
            ],
            "RightBank": [
                "1"
            ]
        },
        {
            "Idx": "3",
            "Transferred": [
                "2"
            ],
            "LeftBank": [
                "0",
                "3",
                "4"
            ],
            "RightBank": [
                "1",
                "2"
            ]
        },
        {
            "Idx": "4",
            "Transferred": [
                "1"
            ],
            "LeftBank": [
                "0",
                "1",
                "3",
                "4"
            ],
            "RightBank": [
                "2"
            ]
        },
        {
            "Idx": "5",
            "Transferred": [
                "0",
                "1"
            ],
            "LeftBank": [
                "3",
                "4"
            ],
            "RightBank": [
                "0",
                "1",
                "2"
            ]
        },
        {
            "Idx": "6",
            "Transferred": [
                "0"
            ],
            "LeftBank": [
                "0",
                "3",
                "4"
            ],
            "RightBank": [
                "1",
                "2"
            ]
        },
        {
            "Idx": "7",
            "Transferred": [
                "3"
            ],
            "LeftBank": [
                "0",
                "4"
            ],
            "RightBank": [
                "1",
                "2",
                "3"
            ]
        },
        {
            "Idx": "8",
            "Transferred": [
                "1"
            ],
            "LeftBank": [
                "0",
                "1",
                "4"
            ],
            "RightBank": [
                "2",
                "3"
            ]
        },
        {
            "Idx": "9",
            "Transferred": [
                "0",
                "1"
            ],
            "LeftBank": [
                "4"
            ],
            "RightBank": [
                "0",
                "1",
                "2",
                "3"
            ]
        },
        {
            "Idx": "10",
            "Transferred": [
                "2"
            ],
            "LeftBank": [
                "2",
                "4"
            ],
            "RightBank": [
                "0",
                "1",
                "3"
            ]
        },
        {
            "Idx": "11",
            "Transferred": [
                "4"
            ],
            "LeftBank": [
                "2"
            ],
            "RightBank": [
                "0",
                "1",
                "3",
                "4"
            ]
        },
        {
            "Idx": "12",
            "Transferred": [
                "1"
            ],
            "LeftBank": [
                "1",
                "2"
            ],
            "RightBank": [
                "0",
                "3",
                "4"
            ]
        },
        {
            "Idx": "13",
            "Transferred": [
                "2"
            ],
            "LeftBank": [
                "1"
            ],
            "RightBank": [
                "0",
                "2",
                "3",
                "4"
            ]
        },
        {
            "Idx": "14",
            "Transferred": [
                "0"
            ],
            "LeftBank": [
                "0",
                "1"
            ],
            "RightBank": [
                "2",
                "3",
                "4"
            ]
        },
        {
            "Idx": "15",
            "Transferred": [
                "0",
                "1"
            ],
            "LeftBank": "",
            "RightBank": [
                "0",
                "1",
                "2",
                "3",
                "4"
            ]
        }
    ]
}
