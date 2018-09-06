# `systematicstools`

  This package provides a framework for writing, using, and interpreting the output of modular tools for propagating systematic uncertainties within an experimental simulation tool chain.

## Introduction

  Experimental physics is lousy with models. The 'event' signatures recorded in
  detectors are very often ambiguous and thus extensive simulation is needed to
  interpret what is observed. If these simulations are incorrect (they are),
  then our interpretation of the underlying physics will be similarly
  incorrect. To account for this, we make plausible variations of many
  components of the simulation and hope (assume) that somewhere within the high
  dimensional space of variations, is a model that describes nature well enough
  for our purposes. Because of the computing time involved with running a full
  simulation, it is often attractive to be able to make such variations to
  a set of simulated observations at 'analysis-', or interpretation-, time.
  Sometimes these variations can be exactly parameterized and thus the varied, simulated observations are statistically equivalent to having re-run the entire simulation. Sometimes they are approximate.

  This package aims to provide a framework for developing and using tools that provide these variations. The framework itself aims to contain no assumptions on the experiment or physics analysis being performed, these are left up to specific implementations that reside in dependent packages.

## Where to start

  For a short primer on the main moving parts within `systematicstools`, see [doc/MovingParts](systematicstools/doc/MovingParts.md).

  For a description of the two levels of configuration file, see [doc/ToolConfiguration](systematicstools/doc/ToolConfiguration.md) and [doc/ParameterHeaders](systematicstools/doc/ParameterHeaders.md).

  For an end-to-end example of how to configure, generate, and interpret some example event responses, see [doc/ExampleSystProvider](systematicstools/doc/ExampleSystProvider.md).

  For tips on how to begin writing a new systematic provider, see [doc/WritingAProvider](systematicstools/doc/WritingAProvider.md).