#ifndef  SDAIHEADER_SECTION_SCHEMA_SCHEMA_H
#define  SDAIHEADER_SECTION_SCHEMA_SCHEMA_H
// This file was generated by exp2cxx.  You probably don't want to edit
// it since your modifications will be lost if exp2cxx is used to
// regenerate it.

#ifdef STEPCODE_LOGGING
#include <sys/time.h>
#endif

#include "export.h"
#include "core/sdai.h"
#include "core/Registry.h"
#include "core/STEPaggregate.h"
#include "core/STEPundefined.h"
#include "core/ExpDict.h"
#include "core/STEPattribute.h"
#include "core/complexSupport.h"

#include "editor/SdaiHeaderSchemaClasses.h"
#include "editor/SdaiHeaderSchema.h"

STEPCODE_EDITOR_EXPORT void HeaderSchemaInit( Registry & );
STEPCODE_EDITOR_EXPORT void HeaderInitSchemasAndEnts( Registry & );
STEPCODE_EDITOR_EXPORT void SdaiHEADER_SECTION_SCHEMAInit( Registry & r );

#endif