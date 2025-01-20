/*
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
*/

// Using hashCode from https://gist.github.com/hyamamoto/fd435505d29ebfa3d9716fd2be8d42f0
function hashCode(s) {
  var h = 0, l = s.length, i = 0;
  if ( l > 0 )
    while (i < l)
      h = (h << 5) - h + s.charCodeAt(i++) | 0;
  return h;
}

var hashScenario, issuesValidityTimer,
    scenarios = [
  "annoyingNeighbors",
  "athletesAndCoaches",
  "bearGorillasChickenAndMouse",
  "bridgeAndTorch",
  "catchingTheTrain",
  "catsAndDogs",
  "chessKingsQueensAndPawns",
  "copsAndRobbers",
  "familiesWithAChild",
  "familyAndBag",
  "familyPlusOldCouple",
  "farmerFamilyAndPets",
  "fourSoldiers",
  "lionsRaccoonsAndSquirrels",
  "merchantsAndRobbers",
  "mothersWithChildren",
  "parents4ChildrenCopAndThief",
  "robbersAndBags",
  "travelersWithBags",
  "weights",
  "wolfGoatCabbage",
  "wolvesCowsGoat",
  "workersAndChildren"
];

function selectedScenario() {
  $('#knownPuzzles').disabled = true;
  var selection = $('#knownPuzzles')[0].value;
  if(selection < 0) {
    $("#scenarioData").val('');
    $('#knownPuzzles').disabled = false;
    return
  }

  $("#scenarioData").load("https://raw.githubusercontent.com/FlorinTulba/RiverCrossing/master/Scenarios/"
    +scenarios[selection]+".json", function(response, status, xhr) {
      if(status == 'error') {
        $('#knownPuzzles').val(-1);
        $("#scenarioData").val('');
        $("#issues").html("Couldn't load: "
          + "https://raw.githubusercontent.com/FlorinTulba/RiverCrossing/master/Scenarios/"
          + scenarios[selection]+".json");
        $("#issuesBox").css({'display':'inline-block'});
        $('#knownPuzzles').disabled = false;
        return
      }

      $("#scenarioData").val($("#scenarioData").text());
      $('#knownPuzzles').disabled = false;
    });
}

function hideIssues() {
  $("#issuesBox").css({'display':'none'});
}

function showIssues(content) {
  $("#issues").html(content);
  $("#issuesBox").css({'display':'inline-block', 'width':'100%'});
  var lineInfo = content.match(/line\s[1-9]\d*/)
  if(lineInfo != null) {
    var startPos = -1, endPos, text = $("#scenarioData").val();
    for(var issuesLine = Number(lineInfo[0].substring(5)),
        crtLine = 1; crtLine < issuesLine; crtLine++) {
      startPos = text.indexOf('\n', startPos+1)
      if(startPos < 0) {
        if(crtLine + 1 != issuesLine)
          console.log('The scenario has only '+crtLine
            +' lines, while the error is signaled on line '+issuesLine+'!')
        break
      }
    }
    endPos = text.indexOf('\n', ++startPos)
    if(endPos < 0)
      endPos = text.length
    
    var theTextArea = $("#scenarioData")[0]

    // Scroll to the line with the issues
    theTextArea.blur()
    $("#scenarioData").val(text.substring(0, endPos-1))
    theTextArea.focus() // moves to the end (where startPos is now)
    theTextArea.scrollTop = 9999999 // IE method
    
    $("#scenarioData").val(text) // set the original text

    if(typeof(theTextArea.selectionStart) != 'undefined') { // Chrome/Firefox
      theTextArea.selectionStart = startPos
      theTextArea.selectionEnd = endPos
    
    } else if(document.selection && document.selection.createRange) { // IE
      theTextArea.select()
      var range = document.selection.createRange()
      range.collapse(true)
      range.moveEnd("character", endPos)
      range.moveStart("character", startPos)
      range.select()
    }
  }

  issuesValidityTimer = setInterval(function() {
    if(hashCode($("#scenarioData").val()) != hashScenario) {
      clearInterval(issuesValidityTimer)
      hideIssues()
    }
  }, 1000)
}

$("document").ready(function() {
  if(sessionStorage.scenarioData)
    $("#scenarioData").val(sessionStorage.scenarioData);
  
  if(typeof(sessionStorage.interactiveSol) === 'undefined'
      || sessionStorage.interactiveSol == 'true')
    $("#interactiveSolId").attr('checked', 'checked');

  if( ! sessionStorage.foundSolution)
    sessionStorage.foundSolution = 0

  hashScenario = hashCode($("#scenarioData").val());

  if(issues.length > 0 && issues != '{{.}}' && 0 == sessionStorage.foundSolution) {
    console.log(issues);    
    if(issues.match(/^('")/) != null && issues.match(/('")$/) != null)
      issues = issues.slice(1, issues.length-1) // remove wrapping ''
    issues = issues
      .replace(/^[^\s]+\s*-\s*/, '') // skip 'functionName - ', if any (it was logged earlier)
      .replace(/\n$/, '') // remove a trailing \n
      .replace(/<unspecified file>\((\d+)\)/, 'line $1') // line number
      .replace(/(\n)/g, '<br>') // use <br> instead of \n, if any
    if(issues.length > 0) showIssues(issues)
  }

  for(var i=0, lim = scenarios.length; i<lim; ++i)
    $('#knownPuzzles').append(
      '<option value="'+i+'">'+scenarios[i]+'</option>');

  $('#knownPuzzles').change(selectedScenario);

  $('#hideIssues').click(hideIssues);

  $("#formData").submit(function() {
    if($("#scenarioData").val().length == 0)
      return false // empty puzzle data

    clearInterval(issuesValidityTimer)

    if(hashCode($("#scenarioData").val()) != hashScenario) { // altered puzzle data
      sessionStorage.scenarioData = $("#scenarioData").val();
      sessionStorage.foundSolution = 0
    
    } else if(issues.length > 0
        && 0 == sessionStorage.foundSolution) { // same puzzle data, some known issues
      showIssues(issues)
      sessionStorage.interactiveSol = $("#interactiveSolId").is(":checked");
      return false // no submit, just (re)display the known issues
    }

    if(0 != sessionStorage.foundSolution) { // got back after displaying the solution for same puzzle data
      if(sessionStorage.interactiveSol
          == $("#interactiveSolId").is(":checked").toString()) { // Same visualization type
        history.forward()
        return false // no submit, just forward
      }
    }

    // No solution yet or a different solution visualization
    sessionStorage.interactiveSol = $("#interactiveSolId").is(":checked");
  });
});
