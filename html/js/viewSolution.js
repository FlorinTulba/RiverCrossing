/*
 Part of the RiverCrossing project,
 which allows to describe and solve River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 (c) 2018 Florin Tulba (florintulba@yahoo.com)
*/

// Expects a previously defined variable `solution` from `solution.js` !

var
  diurnal, // to show the sun or the  moon
  boat, // to show a boat or a bridge

  // orientation: landscape / portrait
  isLandscape = window.matchMedia("(orientation: landscape)"),
  landscape = true,
  
  leftBank = true, // is the current move starting from the left or right bank
  
  entsCount, // entities count
  entsToMove, // the id-s of the entities to be moved for each transfer
  
  // the largest reference size of an entity (when closest to the view's bottom)
  refEntSz,
  
  // centers of the entities when not moving on each bank and for each orientation
  centersOffsetsL = [], // landscape, banks are symmetrical [brought lots of issues]
  centersOffsetsPBottom = [], centersOffsetsPTop = [], // portrait
  
  // The id-s of the entities from the scenario files might be skipping values or out of order
  // However, they will be displayed exactly in the order from the scenario file,
  // regardless their id-s
  idToIdxMap = {}, idxToIdMap = {}, // maps between entity id and display index
  
  // For each animated move, there are a discrete number of stages
  // In landscape mode, the river is quite large and the crossing seemed to
  // happen much to fast. In that case the delay for each step was increased.
  animationStage = 0, stagesPerSegment = 15, delay = 20,
  
  stateIdx, // index of the reached or about to reach state 

  timer;

// How things shrink based on the depth provided by bot (bottom)
// Don't use with arguments larger than 79
function shrinkFactor(bot) {return 1 - bot / 80.;}

function create_Id_Idx_Maps() {
  for(var idx = 0; idx < entsCount; ++idx) {
    var ent = solution.Entities[idx],
      id = ent.Id;
    idToIdxMap[id] = idx;
    idxToIdMap[idx] = id;
  }
}

/*
  First, the reference entity size must be computed (largest size used
  for any entity).

  For the portrait arrangement, the entities are
  equally spaced horizontally, but just not in front of the raft / bridge.
  Upper limit of this reference size is then 84/entsCount.

  For the landscape arrangement, following aspects appear:
  - accomodate any number of entities
  - be symmetrical for both banks
  - avoid overlapping entities due to horizontal / vertical proximity
  - avoid blocking the direct path towards the raft / bridge
  - avoid covering the entities
    that are boarding the raft / about to traverse the bridge
  - avoid totally covering the house with its Info button

  This can be achieved by placing the entities in 2 rows,
  one at the bottom of the screen and one right below the trees plus
  an entity that is between the rows, at the margin of the view.
  To avoid covering the house, the upper row is shorter.
  Upper limit of this reference size is then 70/(entsCount-1)
  if the bottom row's center is at 10% and the top row's center is at 45%,
  case when the top entities are half size from those near bottom

  The reference needs to be decreased then until:
  2*floor(32/refEntSz) >= entsCount 
    and
  1+floor(30/refEntSz)+floor(40/refEntSz) >= entsCount
*/
function computeStaticCenterOffsets() {
  for(refEntSz = Math.min(
        Math.floor(84/entsCount),
        Math.floor(70/(entsCount-1))); // ok, since entsCount >= 3
    refEntSz > 0 &&
    (
      2 * Math.floor(42/refEntSz) < entsCount
      ||
      1 + Math.floor(30/refEntSz) + Math.floor(40/refEntSz) < entsCount
    );
    --refEntSz);

  if(refEntSz > 20) refEntSz = 20; // ensure bottom row's center stays on 10% from floor
  // We might generate more centers than entities

  var halfRefEntSz = refEntSz / 2.;

  // The coordinates for portrait:

  for(var offsetLeft = halfRefEntSz; offsetLeft + halfRefEntSz < 42.01; offsetLeft += refEntSz) {
    centersOffsetsPBottom.push([offsetLeft, 10]);
    centersOffsetsPTop.push([25 + offsetLeft/2, 45]);
  }
  // gap for not placing anything in front of the raft/bridge, unless boarding
  for(var offsetLeft = 58 + halfRefEntSz; offsetLeft + halfRefEntSz < 100.01; offsetLeft += refEntSz) {
    centersOffsetsPBottom.push([offsetLeft, 10]);
    centersOffsetsPTop.push([25 + offsetLeft/2, 45]);
  }

  if(centersOffsetsPBottom.length > entsCount) {
    centersOffsetsPBottom = centersOffsetsPBottom.slice(0,entsCount);
    centersOffsetsPTop = centersOffsetsPTop.slice(0,entsCount);
  }

  // The coordinates for landscape:

  // Bottom row
  for(var offset = 30-halfRefEntSz; offset > halfRefEntSz-0.01; offset -= refEntSz)
    centersOffsetsL.push([offset, 10]);

  // Let's position always the center of the middle entity on 30%,
  // which will be above the bottom row and under the house
  var midEntSz = shrinkFactor(30) * refEntSz;
  centersOffsetsL.push([midEntSz/2., 30]);

  // Top row
  var quarterRefEntSz = halfRefEntSz/2.,
    topRowOffsets = [];
  for(var offset = 30-quarterRefEntSz; offset > quarterRefEntSz + 9.99; offset -= halfRefEntSz)
    topRowOffsets.push([offset, 45]);

  // The top row clock-wise, but the centersOffsetsL were computed anti-clock-wise
  centersOffsetsL = centersOffsetsL.concat(topRowOffsets.reverse());

  if(centersOffsetsL.length > entsCount)
    centersOffsetsL = centersOffsetsL.slice(0,entsCount);
}

function timeOfDay() {
  if(diurnal) {
    $(".nocturnal").removeClass('nocturnal').addClass('diurnal');
  } else {
    $(".diurnal").removeClass('diurnal').addClass('nocturnal');
  }
}

function updateRaftOrBridgeAndArrow() {
  var selector = '#';
  if(leftBank) {
    $("#raft.landscape").
      css({"bottom":"25%", "width":"15%", "left":"22.5%", "right":"auto",
        "z-index": "76"});
    $("#raft.portrait").
      css({'bottom':'20%', "width":"15%", "left":"42.5%", "right":"auto",
        "z-index": "81"});
    $("#towardsLeft.landscape").css('display', 'none');
    $("#towardsLeft.portrait").css('display', 'none');
    selector += "fromLeft";
  } else {
    $("#raft.landscape").
      css({"bottom":"25%", "width":"15%", "left":"auto", "right":"22.5%",
        "z-index": "76"});
    $("#raft.portrait").
      css({'bottom':'39%', "width":"10%", "left":"45.5%", "right":"auto",
        "z-index": "62"});
    $("#fromLeft.landscape").css('display', 'none');
    $("#fromLeft.portrait").css('display', 'none');
    selector += "towardsLeft";
  }
  if( ! boat) {
    if(landscape)
      selector += ".landscape";
    else
      selector += ".portrait";

    $(selector).css('display', 'block');
  } else {
    $(selector + '.landscape').css('display', 'none');
    $(selector + '.portrait').css('display', 'none');
  }
}

function displayBoatOrBridge() {
  if(boat) {
    $("#bridgeLandscape").css('display', 'none');
    $("#bridgePortrait").css('display', 'none');
    if(landscape)
      $("#raft.portrait").removeClass('portrait').addClass('landscape');
    else
      $("#raft.landscape").removeClass('landscape').addClass('portrait');
    $("#raft").css('display', 'block');
  } else {
    $("#raft").css('display', 'none');
    if(landscape)
      $("#bridgeLandscape").css('display', 'block');
    else
      $("#bridgePortrait").css('display', 'block');
  }

  updateRaftOrBridgeAndArrow();
}

// Caution: This orientation change can happen while animating a move
function toggleOrientation() {
  if(landscape) {
    $("#next.portrait, #undo.portrait").removeClass('portrait').addClass('landscape');
    if(boat)
      $("#raft.portrait").removeClass('portrait').addClass('landscape');
    $(".portrait").css('display', 'none');
    $(".landscape").css('display', 'block');
  } else {
    $("#next.landscape, #undo.landscape").removeClass('landscape').addClass('portrait');
    if(boat)
      $("#raft.landscape").removeClass('landscape').addClass('portrait');
    $(".landscape").css('display', 'none');
    $(".portrait").css('display', 'block');
  }
  updateRaftOrBridgeAndArrow();
  displayBoatOrBridge();

  solution.Entities.forEach(function(ent) {updateEnt(ent.Id)});
}

// Populates div id land with trees
function createForest() {
  var afterWhat = 'riverLandscape',
    idx = 1, maxTreeSz;

  // bot stands for the distance (as percentage) from the bottom of the land zone
  for(var bot = 99; bot > 50; bot -= Math.ceil(minTreeSz)) {
    var treesToCreate = 5 + 95 * (bot-50)/49, // 100 trees for bot 99; 5 trees for bot 50
      maxTreeSz = (103-bot)/2, minTreeSz = Math.max(1, maxTreeSz/2),
      zIdx = Math.round(100-bot),
      minLeft, minRight; // margins to avoid placing trees in the river
    if(bot >= 79) { // the river has an elbow at 79% from bottom
      minLeft = 49 + 8 * (bot-79)/20; // 49 for bot 79 and 57 for bot 99
      minRight = 49 + 9 * (99-bot)/20; // 58 for bot 79 and 49 for bot 99
    } else {
      minLeft = 49 + 8 * (79-bot)/20; // 49 for bot 79 and 57 for bot 50
      minRight = 58 + 6 * (79-bot)/29; // 58 for bot 79 and 64 for bot 50
    }
    var restMinLeft = 100-minLeft, restMinRight = 100-minRight;

    // generate the trees from this depth (bot)
    for(var tree = 0; tree < treesToCreate; ++tree, afterWhat = 'tree'+idx, ++idx) {
      // floor not necessary
      var treeSz = minTreeSz + (maxTreeSz-minTreeSz+1) * Math.random();

      var marginName, marginVal;

      // The if-else from below provides trees for both landscape & portrait perspectives
      if(Math.random() < 0.5) { // create on the left bank
        // Trees with parts beyond the left margin don't generate horiz scrollbar
        marginVal = minRight + restMinRight * Math.random(); // floor not necessary
        marginName = 'right';
      
      } else { // create on the right bank
        // Must stay away of the right margin, to avoid the horiz. scrollbar
        // Any value above this produces tree with parts beyond the right margin
        var maxMargin = 100-treeSz;
        marginVal = Math.min(maxMargin,
          minLeft + restMinLeft * Math.random()); // floor not necessary
        marginName = 'left';
      }

      $("#" + afterWhat).after('<img id="tree' + idx
        + '" style="position:absolute;bottom:' + bot
        + '%;' + marginName + ':' + marginVal + '%;max-height:' + treeSz
        + '%;max-width:' + treeSz + '%;z-index:'+ zIdx
        + ';" src="../Scenarios/images/tree'
        + Math.floor(1 + 2 * Math.random()) + '.png" alt="A tree">');
    }  

    // Following 5 trees are for the portrait perspective only
    // They fill the space used by the river in the landscape perspective
    for(var treePortrait = 0,
        left = 100-minRight, delta = (minLeft - left)/4; treePortrait < 5; ++treePortrait,
        afterWhat = 'tree'+idx, ++idx,
        left += delta) {
      // floor not necessary
      var treeSz = minTreeSz + (maxTreeSz-minTreeSz+1) * Math.random();
      $("#" + afterWhat).after('<img id="tree' + idx
        + '" class="portrait" style="position:absolute;bottom:' + bot
        + '%;left:' + (left-delta) + '%;max-height:' + treeSz
        + '%;max-width:' + treeSz + '%;z-index:'+ zIdx
        + ';" src="../Scenarios/images/tree'
        + Math.floor(1 + 2 * Math.random()) + '.png" alt="A tree">');
    }
  }
}

// Called at the end of a move, to update the reported step index among others
function updateStatus() {
  var state = solution.Moves[stateIdx];
  $("#stepIdx").text(stateIdx);
  $("#otherDetails").text(state.OtherDetails || '');

  animationStage = 0;
  entsToMove = [];

  if(stateIdx == 0) $("#undo").addClass('disabled');
  else $("#undo").removeClass('disabled');
  
  if(stateIdx == solution.Moves.length - 1) $("#next").addClass('disabled');
  else $("#next").removeClass('disabled');

  if(state.hasOwnProperty('NightMode'))
    diurnal = ! state.NightMode;
  else diurnal = true;
  timeOfDay();
}

// Each displayed entity provides a tooltip with details about itself
function entDetails(ent) {
  var out;
  if(ent.hasOwnProperty('CanRow'))
    out = "Can row: " + ent.CanRow;
  else if(ent.hasOwnProperty('CanTackleBridgeCrossing'))
    out = "Can tackle bridge crossing: " + ent.CanTackleBridgeCrossing;
  else
    out = "Cannot cross the river independently";

  if(ent.hasOwnProperty('StartsFromRightBank') && ent.StartsFromRightBank)
    out += "; Starts from right/upper bank";

  if(ent.hasOwnProperty('Weight'))
    out += "; Weighs " + ent.Weight;
  out = ent.Name + ": {" + out + "}";
  return out;
}

// n-th point between from and to when there is a total count of `steps`
// Works for multi-dimensional points (when from and to are arrays)
function nthBetween(nth, from, to, steps = stagesPerSegment) {
  if(from.hasOwnProperty('length') && to.hasOwnProperty('length')) {
    var len = from.length;
    if(len != to.length) {
      alert('nthBetween with arrays parameter of different length');
      return undefined;
    }

    var out = [];
    for(var i=0; i<len; ++i)
      out.push(from[i] + (to[i] - from[i]) * nth / steps);

    return out;
  }
  return from + (to - from) * nth / steps;
}

// The raft/bridge have a width of 15% from the view's width
// except in landscape mode, when the upper bank requires proportionally smaller sizes
function raftBridgeWidth() {
  var width1 = 15, width2 = 15,
    step = (animationStage-1) % stagesPerSegment + 1;
  if( ! landscape) {
    if(leftBank) width2 = 10;
    else width1 = 10;
  }

  return nthBetween(step, width1, width2);
}

/*
  Repositions, resizes, reassigns z-index and transparency for entities
  either during the animation of a move (when the entity belongs to entsToMove),
  or when the view's orientation changes (when all entities get updated).
  This means that the entities part of the animation get updated twice during an
  orientation change. It is safest so, as the latest call always uses the correct
  orientation. Otherwise an animated entity might remain set for the previous
  orientation and disregard the second call which would correct the orientation.
*/
function updateEnt(entId, animationCause = true) {
  var idle = (entsToMove.indexOf(entId) < 0),
    entIdx = idToIdxMap[entId],
    ent = solution.Entities[entId],
    movesFromLeftBank = ent.MovesFromLeftBank,

    // szReduce is 0 or a negative small integer
    // which decreases the size & importance of that entity
    szReduce = ent.hasOwnProperty('Image') ? (ent.Image.Size || 0) : 0,
    centersOffsets,
    bottom, lateral, left="auto", right="auto", sz, zIdx,
    beforeReachingLeftBank =
      (movesFromLeftBank && animationStage <= stagesPerSegment)
      || ( ! movesFromLeftBank &&  animationStage > stagesPerSegment);

  if(landscape) centersOffsets = centersOffsetsL;
  else
    centersOffsets = 
      ((idle && movesFromLeftBank) || ( ! idle && beforeReachingLeftBank))
        ? centersOffsetsPBottom : centersOffsetsPTop;

  var pos = centersOffsets[entIdx].concat(); // must clone, to avoid overwriting
  if(animationStage > 0 && ! idle) {
    var centerEmbarked, oppositeCenterEmbarked,
      step = (animationStage-1) % stagesPerSegment + 1;
    
    var bottomEntInBoat;
    if(landscape) {
      bottomEntInBoat = 26;
      var entSzInBoat = Math.max(1,
        refEntSz * shrinkFactor(bottomEntInBoat) * (1 + 0.15 * szReduce));
      
      centerEmbarked = [30,
        bottomEntInBoat + entSzInBoat/2]; // lateral is symmetrical 30
      oppositeCenterEmbarked = [100-30, centerEmbarked[1]];
    
    } else {
      if(movesFromLeftBank == (animationStage <= stagesPerSegment))
        bottomEntInBoat = 21;
      else bottomEntInBoat = 40;
      var entSzInBoat = Math.max(1,
        refEntSz * shrinkFactor(bottomEntInBoat) * (1 + 0.15 * szReduce));
      
      centerEmbarked = [50, bottomEntInBoat + entSzInBoat/2];
      oppositeCenterEmbarked = [50, (21+40-bottomEntInBoat) + entSzInBoat/2];
    }

    if(animationStage <= stagesPerSegment) { // approaching raft/bridge
      pos = nthBetween(step, pos, centerEmbarked);
    
    } else if(animationStage > 2 * stagesPerSegment) { // moving towards assigned positions
      pos = nthBetween(step, centerEmbarked, pos);
    
    } else { // on the raft
      pos = nthBetween(step, oppositeCenterEmbarked, centerEmbarked);
    }
  }

  // this is actually a center, but still can be used below to get a raw size the entity
  bottom = pos[1];
  lateral = pos[0];
  sz = Math.max(1,
    refEntSz *
    shrinkFactor(bottom) * // resulted factor between 0..1 (bottom is < 80)
    (1 + 0.15 * szReduce)); // szReduce is 0 or a negative small integer
  var halfSz = sz/2,

    // landscape is lateral-symmetrical; portrait preserves lateral values
    lateralToSet = 
      (landscape && ((idle && ! movesFromLeftBank)
                    || ! (idle || beforeReachingLeftBank)))
        ? "right" : "left";
  lateral -= halfSz;
  bottom -= halfSz;
  zIdx = Math.round(100 - bottom);

  // Entities are placed always at left within thei max-width. Corrected by
  // placing the entities within their max-width closer to the closest lateral border
  if(lateral > 50) {
    lateral = 100 - lateral - sz;
    if(lateralToSet == "right") lateralToSet = "left";
    else lateralToSet = "right"
  }

  if(lateralToSet == "left") left = lateral + '%';
  else right = lateral + '%';

  var opacity = 1.; // allow recognizing most of the entities in a crowded boat
  if( ! idle &&
    (animationStage >= stagesPerSegment && animationStage <= 2*stagesPerSegment))
    opacity = 0.5;

  $("#ent" + entIdx).
    css({"bottom": bottom + "%",
      "left": left,
      "right": right,
      "max-width": sz + "%",

      // trick to let entities be more visible while still fitting horizontally
      "max-height": (2.5*sz) + "%", // most entities fit within tall rectangles
      "opacity": opacity,
      "z-index": zIdx});
}

// Presents the entities from the solution
function createEntities() {
  var afterWhat = "bridgePortrait";
  var state = solution.Moves[stateIdx];

  for(var idx = 0; idx < entsCount; afterWhat = 'ent' + idx++) {
    var entId = idxToIdMap[idx],
      ent = solution.Entities[entId],
      url = ent.hasOwnProperty('Image') ? (ent.Image.Url || '') : '',
      onLeftBank = (state.LeftBank.indexOf(ent.Id) >= 0);

    ent.MovesFromLeftBank = onLeftBank;
    $("#" + afterWhat).after('<img id="ent' + idx
      + '" style="position:absolute;bottom:0;z-index:0" src="' + url
      + '" alt="' + ent.Name + '" data-toggle="tooltip" title="'
      + entDetails(ent) + '">');
    updateEnt(entId);
  }
}

// Returns to the previous solution step, if any
// Undo is performed without animations, to ensure forward and backward steps
// are distinct enough to avoid any confusion about what just happened
function undo() {
  if(stateIdx != 0 && animationStage == 0) {
    $("#next").addClass('disabled');
    $("#undo").addClass('disabled');
    entsToMove = solution.Moves[stateIdx--].Transferred;
    entsToMove.forEach(function(id) {
      var ent = solution.Entities[idToIdxMap[id]];
      ent.MovesFromLeftBank = ! ent.MovesFromLeftBank;
      updateEnt(id);
    });
    leftBank = ! leftBank;
    updateRaftOrBridgeAndArrow();
    updateStatus();
  }
}

// Last segment of the animation: from the raft/bridge to the assigned positions
function moveToAssignedPositions() {
  if(++animationStage <= 3*stagesPerSegment) {
    entsToMove.forEach(function(id) {updateEnt(id);});

    if(animationStage == 2 * stagesPerSegment + 1)
      timer = setInterval('moveToAssignedPositions()', delay);
  } else {
    clearInterval(timer);
    timer = undefined;
    entsToMove.forEach(function(id) {
      var ent = solution.Entities[idToIdxMap[id]];
      ent.MovesFromLeftBank = ! ent.MovesFromLeftBank;
    });
    updateStatus();
  }
}

// The boat moves towards the target bank at the rate dictated by animationStage
function animateRaft() {
  var centerEmbarked, oppositeCenterEmbarked,
    step = (animationStage-1) % stagesPerSegment + 1;
  
  var bottomBoat, leftBoat;
  if(landscape) {
    bottomBoat = 25;
    var leftBoat;
    if(leftBank) leftBoat = 22.5;
    else leftBoat = 62.5;
    centerEmbarked = [leftBoat, bottomBoat];
    oppositeCenterEmbarked = [100-15-leftBoat, bottomBoat];
  
  } else {
    var leftBoat1, leftBoat2;
    if(leftBank) {
      bottomBoat = 20;
      leftBoat1 = 42.5;
      leftBoat2 = 45.5;
    } else {
      bottomBoat = 39;
      leftBoat1 = 45.5;
      leftBoat2 = 42.5;
    }
    centerEmbarked = [leftBoat1, bottomBoat];
    oppositeCenterEmbarked = [leftBoat2, 39+20-bottomBoat];
  }

  var boatPos = nthBetween(step, centerEmbarked, oppositeCenterEmbarked);
  $("#raft").
    css({"bottom": boatPos[1] + "%",
      "width": raftBridgeWidth() + "%",
      "left": boatPos[0] + "%",
      "right": "auto",
      "z-index": Math.ceil(101-boatPos[1])});
}

// Second segment of the animation involves the actual river traversal
function crossTheRiver() {
  if(++animationStage <= 2*stagesPerSegment) {
    entsToMove.forEach(function(id) {updateEnt(id);});

    if(boat) animateRaft();

    if(animationStage == stagesPerSegment + 1)
      timer = setInterval('crossTheRiver()',
        // hack to ensure the wider stretch of the river (for the landscape orientation)
        // is traversed at a normal pace. Otherwise it appears too fast.
        delay * (landscape ? 4 : 1));
  } else {
    clearInterval(timer);
    timer = undefined;
    --animationStage;
    leftBank = ! leftBank;
    updateRaftOrBridgeAndArrow();
    moveToAssignedPositions();
  }
}

// First segment of the animation: from the assigned positions to the raft/bridge
function approachRaftOrBridge() {
  if(++animationStage <= stagesPerSegment) {
    entsToMove.forEach(function(id) {updateEnt(id);});

    if(animationStage == 1) timer = setInterval('approachRaftOrBridge()', delay);
  } else {
    clearInterval(timer);
    timer = undefined;
    --animationStage;
    crossTheRiver();
  }
}

// Performs the next animated move, if any
function next() {
  var state = solution.Moves[stateIdx];
  if(animationStage == 0 && stateIdx != solution.Moves.length-1) {
    $("#next").addClass('disabled');
    $("#undo").addClass('disabled');

    entsToMove = solution.Moves[++stateIdx].Transferred || [];
    if(entsToMove == []) {
      alert("Transferred property is missing from solution.Moves["
        + stateIdx + "]. Cannot continue!");
      return;
    }

    timer = undefined;
    approachRaftOrBridge();
  }
}

// Caution: This orientation change can happen while animating a move
function orientationChangeHandler(evt) {
  if(landscape != evt.matches) {
    landscape = ! landscape;
    toggleOrientation();
  }
}

/*
  The solution object must have a description of the puzzle, which can be
  presented in a modal window.
  There are a few introductory phrases, then a part describing the constraints
  as a list.
*/
function extractPuzzleDescr() {
  var descr = solution.ScenarioDescription,
    lines = descr.length;
  var out = '', foundList = false;
  for(var i=0; i<lines; ++i) {
    var line = descr[i];
    if(line == "Constraints:") {
      foundList = true;
      out += "<p>" + line + "<ul>";
      continue;
    }

    if(foundList) {
      out += "<li>" + line.replace(/^-\s*/, '') + "</li>";
      continue;
    }

    out += line;
  }
  if(foundList)
    out += "</ul><p>";

  return out;
}

// Ensures the received solution object appears ok
function correctInput() {
  if(typeof solution === 'undefined') {
    alert('Please define a solution before loading this file!');
    return false;
  }

  if( ! solution.hasOwnProperty('ScenarioDescription')) {
    alert('The defined solution must define a ScenarioDescription property!');
    return false;
  }

  if( ! solution.hasOwnProperty('Entities')) {
    alert('The defined solution must define a Entities property!');
    return false;
  }
  if( solution.Entities.length > 17) {
    alert('The entities are barely visible for more than 18 entities!');
    return false;
  }

  if( ! solution.hasOwnProperty('Moves')) {
    alert('The defined solution must define a Moves property!');
    return false;
  }

  return true;
}

$("document").ready(function() {
  if( ! correctInput()) return; // problems with the provided solution object

  isLandscape.addListener(orientationChangeHandler);

  $('#infoText').html(extractPuzzleDescr());
  
  if(solution.hasOwnProperty('Bridge')) boat = ! solution.Bridge;
  else boat = true;

  stateIdx = 0;

  createForest();
  updateRaftOrBridgeAndArrow();
  displayBoatOrBridge();

  entsCount = solution.Entities.length;
  create_Id_Idx_Maps();
  computeStaticCenterOffsets();

  $("#stepsCount").text(solution.Moves.length - 1);
  updateStatus();
  createEntities();
  orientationChangeHandler(isLandscape);

  $('[data-toggle="tooltip"]').tooltip();

  $("#next").click(next);
  $("#undo").click(undo);
});
