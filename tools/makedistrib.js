var files = new Array("yamy.ini", "104.mayu", "109.mayu", "default.mayu", "emacsedit.mayu", "104on109.mayu", "109on104.mayu", "dot.mayu", "workaround.mayu", "workaround.reg", "readme.txt", "yamy.exe", "yamy32", "yamy32.dll", "yamyd32", "yamy64", "yamy64.dll");

var config = WScript.Arguments.Item(0); // "Debug" or "Release"
var version = WScript.Arguments.Item(1); // x.yz
if (config == null | version == null) {
	throw new Error("usage: CScirpt.exe makedistrib.js {Debug | Release} <version>");
}

var targetDir = "..\\" + config + "\\";
var pkgFile = "yamy-" + version + ".zip";

function ProcessFiles(dir, files, process) {
    for (var i = 0; i < files.length; i++) {
	process(dir, files[i]);
    }
}

var RemoveFile = function(dir, name) {
    var path =  dir + name;
    if (fso.FileExists(path)) {
	fso.DeleteFile(path);
    }
};

var fso = WScript.CreateObject("Scripting.FileSystemObject");
if (fso == null) {
	throw new Error("can't create File System Object!");
}

var shell = WScript.CreateObject("Shell.Application");
if (fso == null) {
	throw new Error("can't create Shell Application Object!");
}

if (fso.FolderExists(targetDir) == false) {
    fso.CreateFolder(targetDir);
}

RemoveFile(targetDir, pkgFile);

var file = fso.CreateTextFile(targetDir + pkgFile, true);
file.Write("PK\x05\x06\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
file.Close();

var targetZip = shell.NameSpace(fso.GetAbsolutePathName(targetDir + pkgFile));

var PackFile = function(dir, name) {
    var path =  dir + name;
    if (fso.FileExists(path) == false) {
	RemoveFile(targetDir, pkgFile);
	throw new Error("can't pack " + path + "!");
    }
    var item = shell.NameSpace(fso.GetAbsolutePathName(path) + "\\..\\").ParseName(name);
    var count = targetZip.Items().Count;
    targetZip.CopyHere(item);
    while (targetZip.Items().Count != count + 1) {
	WScript.Sleep(100);
    }
};

ProcessFiles(targetDir, files, PackFile);
