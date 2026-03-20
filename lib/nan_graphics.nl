// nan_graphics.nl
// Declarative NanLang graphics prelude.
// Concatenate this file with a scene file to emit a linear UI boot sequence.

NAN3;

// Application bootstrap
ui app begin;

// Primary window
ui window create 1 1280 720;
ui window title 1 "NanGraph Dashboard";

// Layout and static widgets
ui layout vertical 1 8;
ui label 1 10 "NanGraph ready";
ui button 1 30 "Refresh";
ui slider 1 20 0 100 50;

// Close the application gracefully
ui app end;
