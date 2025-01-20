/*
 This RiverCrossing project (https://github.com/FlorinTulba/RiverCrossing)
 allows describing and solving River Crossing puzzles:
  https://en.wikipedia.org/wiki/River_crossing_puzzle

 Required libraries:
 - Boost (>=1.67) - https://www.boost.org
 - Microsoft GSL (>=4.0) - https://github.com/microsoft/GSL

 (c) 2018-2025 Florin Tulba (florintulba@yahoo.com)
*/

/*
Building the executable:

go build [-C <ProjectFolder>/GoServer/cmd/RiverCrossingServer/] -buildvcs=false

-C allows launching the build directly from <ProjectFolder>,
-buildvcs=false instructs the builder to omit version control information stamping

The executable may be launched from <ProjectFolder>, like:

<ProjectFolder>/startWebServer.(sh|bat)

The paths of the actual RiverCrossing[.exe] solvers are:
<ProjectFolder>/x(32|64|_arm)/(msvc|[clan]g++)/Release/RiverCrossing[.exe]

If there are more solvers available, the server selects the most recent one.

MSYS/Cygwin-compiled solvers can serve only to servers launched exactly from the
same environment, to ensure the PATH contains all necessary folders.

The link to use is:

http://localhost:8080/RiverCrossing
*/
package main

import (
	"errors"
	"fmt"
	"html/template"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"strings"
	"syscall"
	"time"
)

var OS = runtime.GOOS
var sep = string(os.PathSeparator)

// Optional extension of the solver executable
func solverExt() string {
	if OS == "windows" {
		return ".exe"
	}
	return ""
}

// One of x(_arm|64|32), as the solver is saved within <ProjectFolder>/x(_arm|64|32)/...
func solverBaseDirName() string {
	if runtime.GOARCH == "arm" {
		return "x_arm"
	}
	if runtime.GOARCH == "amd64" {
		return "x64"
	}
	return "x32"
}

var theSolverBaseDirName = solverBaseDirName()

// Determines <ProjectFolder>, looking for theSolverBaseDirName
func projDir() string {
	d, _ := os.Getwd()
	for {
		if _, err := os.Stat(d + sep + theSolverBaseDirName); err == nil {
			break
		}

		oldD := d
		d = filepath.Dir(d)
		if oldD == d {
			return ""
		}
	}
	return d
}

var projectDir = projDir()

// Folder containing css and js files, html templates, images and the web server
var webFolder = projectDir + sep + "GoServer" + sep + "web"

// path of the newest solver available; empty if none
var solverPath string

// html templates for the web server
var templates *template.Template

// Determines solverPath as the path of the newest solver available
func selectSolver() string {
	baseDir := projectDir + sep + theSolverBaseDirName + sep
	solverSuffix := sep + "Release" + sep + "RiverCrossing" + solverExt()

	relPath1 := baseDir + "g++" + solverSuffix
	relPath2 := baseDir + "clang++" + solverSuffix
	relPath3 := baseDir + "msvc" + solverSuffix

	found := false

	mt1 := time.Time{}
	if fileInfo, err := os.Stat(relPath1); err == nil {
		mt1 = fileInfo.ModTime()
		found = true
	}

	mt2 := time.Time{}
	if fileInfo, err := os.Stat(relPath2); err == nil {
		mt2 = fileInfo.ModTime()
		found = true
	}

	mt3 := time.Time{}
	if fileInfo, err := os.Stat(relPath3); err == nil {
		mt3 = fileInfo.ModTime()
		found = true
	}

	if !found {
		return ""
	}

	if mt1.After(mt2) && mt1.After(mt3) {
		return relPath1
	}
	if mt2.After(mt1) && mt2.After(mt3) {
		return relPath2
	}
	return relPath3
}

// What to do when the call to the solver goes wrong
func processWaitStatus(waitSt syscall.WaitStatus) (out string) {
	out = ""
	if waitSt.Signaled() {
		sig := waitSt.Signal()
		out += fmt.Sprintf("\nSignaled by: %s (%d)", strings.ToUpper(sig.String()), sig)
		if sig == syscall.SIGABRT {
			// Darwin & Android - missing shared library (SIGABRT)
			if OS == "android" {
				out += fmt.Sprint("\nOne/several required shared libraries might not be found!\n" +
					"Check each library reported by command: ldd selectedSolver!")

			} else if OS == "darwin" {
				out += fmt.Sprint("\nOne/several required shared libraries might not be found!\n" +
					"Check each library reported by command: otool -L selectedSolver!")
			}
		}
	}
	if waitSt.Stopped() {
		sig := waitSt.StopSignal()
		out += fmt.Sprintf("\nStopped by signal: %s (%d)", strings.ToUpper(sig.String()), sig)
	}
	if waitSt.Continued() {
		out += fmt.Sprint("\nContinued")
	}
	if waitSt.TrapCause() != -1 {
		out += fmt.Sprintf("\nTrap cause: %d", waitSt.TrapCause())
	}
	if waitSt.CoreDump() {
		out += fmt.Sprint("\nCore dumped")
	}
	if waitSt.Exited() {
		exitStatus := waitSt.ExitStatus()
		out += fmt.Sprintf("\nExit status: %d (0x%X)", exitStatus, exitStatus)
		if OS == "windows" {
			if exitStatus == 0x1 {
				// Windows - typical return status after killing the task
				// from the task manager (0x1)
				out += fmt.Sprint("\nTask managers typically return 0x1 for an ended task!")

			} else if int64(exitStatus) == 0xC0000135 || int64(exitStatus) == 0xC0000139 {
				// Windows - STATUS_DLL_NOT_FOUND (0xC0000135)
				// Windows - STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139)
				out += fmt.Sprint("\nOne/several required Entrypoint(s) / DLL(s) were not found!\n" +
					"The PATH variable within Cygwin and MSYS environments is set to " +
					"point to specific required DLL-s.\n" +
					"Make sure programs compiled in MSYS/Cygwin are started " +
					"from the same environments!")
			}

		} else if (exitStatus == 0x7F && OS == "linux") ||
			(exitStatus == 0x1 && OS == "freebsd") {
			// Linux & WSL - missing shared library (0x7F)
			// FreeBSD - missing shared library (0x1)
			out += fmt.Sprint("\nOne/several required shared libraries were not found!\n" +
				"Check each library reported by command: ldd selectedSolver!")
		}

	}
	return
}

// Change characters, paths a.o. in order to become valid html output
func adaptAsHtmlOutput(text string) string {
	// Replace all ` and ' with the corresponding html characters
	re := regexp.MustCompile("'")
	text = re.ReplaceAllString(text, "&#39;")
	re = regexp.MustCompile("`")
	text = re.ReplaceAllString(text, "&#96;")

	return text
}

/*
Interacting with RiverCrossing executable

The solver returns:

	0  when finding a solution => solved = true
	-1 when no solution or when it detects an exception => solved = false

For these exit codes there is clear feedback directly from the solver.
Output variable err is nil for these cases only.
Any other case should provide a non-nil err.
*/
func callSolver(inputReader io.Reader, interactive bool) (out string, solved bool, err error) {
	out, solved, err = "", true, nil

	cmdArg := "interactive"
	if !interactive {
		cmdArg = ""
	}

	cmd := exec.Command(solverPath, cmdArg)
	cmd.Stdin = inputReader

	var combinedOutput []byte
	combinedOutput, err = cmd.CombinedOutput() // Stderr has collected errors

	if combinedOutput != nil {
		out += string(combinedOutput)
	}

	hasProcessState := cmd.ProcessState != nil

	// WaitStatus is either uint32 or a struct containing uint32
	var waitStatus syscall.WaitStatus // zero-initialized in both cases
	didExit := false
	exitStatus := 0
	if hasProcessState {
		waitStatus = cmd.ProcessState.Sys().(syscall.WaitStatus)
		if waitStatus.Exited() {
			didExit = true
			exitStatus = waitStatus.ExitStatus()
		}
	}

	if err == nil {
		// hasProcessState must be true
		if !hasProcessState {
			msg := "When err from cmd.CombinedOutput() is nil, cmd.ProcessState cannot be nil!"
			out += msg
			solved = false
			err = errors.New(msg)
			return
		}

		if didExit && exitStatus == 0 {
			out = adaptAsHtmlOutput(out)
			return // the solver solved the puzzle
		}

		solved = false

		if (exitStatus & 0xFF) == 0xFF {
			out = adaptAsHtmlOutput(out)
			return // the solver couldn't solve the puzzle and returned -1
		}

		out += processWaitStatus(waitStatus)
		err = errors.New("Cannot let err nil when exitStatus is not 0, neither -1!")
		return
	}

	solved = false

	if didExit && exitStatus == 0 {
		msg := "When err != nil, the exitStatus cannot be 0"
		out += msg
		err = errors.New(msg)
		return
	}

	errOut := err.Error()

	if !hasProcessState {
		out += errOut + "\nThe called program might be missing, " +
			"it might have wrong permissions, or it might have " +
			"an unrecognized / invalid format!"
		return
	}

	if (exitStatus & 0xFF) == 0xFF {
		out = adaptAsHtmlOutput(out)
		err = nil
		return // the solver couldn't solve the puzzle and returned -1
	}

	// So far, errOut information seems covered by processWaitStatus report.
	// But there might be cases when it contains useful, non-redundant data.
	out += errOut
	out += processWaitStatus(waitStatus)
	return
}

func post(r *http.Request) (tmpl, out string) {
	input := r.FormValue("scenarioData")
	inputReader := strings.NewReader(input)
	interactive := r.FormValue("interactiveSol") == "on"
	var solved bool
	out, solved, _ = callSolver(inputReader, interactive)

	if solved {
		if interactive {
			tmpl = "interactiveSolution"
		} else {
			tmpl = "listedSolution"
		}

	} else { // detected some issue or no solution
		tmpl = "index"
	}
	return
}

func renderTemplate(w http.ResponseWriter, tmpl, replacement string) {
	err := templates.ExecuteTemplate(w, tmpl+".html",
		replacement)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func editScenario(w http.ResponseWriter, r *http.Request) {
	input := r.FormValue("scenarioData")
	if len(input) == 0 { // scenarioData is empty at start
		renderTemplate(w, "index", "") // just display the index template
		return
	}

	tmpl, replacement := post(r)
	renderTemplate(w, tmpl, replacement)
}

func provideFile(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, webFolder+r.URL.Path)
}

func main() {
	if projectDir == "" {
		fmt.Fprintln(os.Stderr, "Couldn't find the RiverCrossing project folder!")
		return
	}
	if solverPath = selectSolver(); solverPath == "" {
		fmt.Fprintln(os.Stderr, "Couldn't find a solver to launch!")
		return
	}

	fmt.Println("Selected the solver:", solverPath)

	// Make a test call to the solver before offering its services
	if out, _, err := callSolver(strings.NewReader("{}"), false); err != nil {
		fmt.Fprintln(os.Stderr, out)
		return
	}

	templatesFolder := webFolder + sep
	templates = template.Must(template.ParseFiles(
		templatesFolder+"index.html",
		templatesFolder+"interactiveSolution.html",
		templatesFolder+"listedSolution.html"))
	http.HandleFunc("/RiverCrossing", editScenario)
	http.HandleFunc("/css/", provideFile)
	http.HandleFunc("/js/", provideFile)
	http.HandleFunc("/images/", provideFile)

	fmt.Print("Starting the ")
	if OS != "windows" {
		fmt.Print("non-")
	}
	fmt.Println("Windows version of the server. Press Ctrl+C to stop it when done!")

	log.Fatal(http.ListenAndServe(":8080", nil))
}
