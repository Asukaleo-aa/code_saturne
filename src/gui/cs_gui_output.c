/*============================================================================
 * Management of the GUI parameters file: output parameters
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2014 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*----------------------------------------------------------------------------*/

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

/*----------------------------------------------------------------------------
 * libxml2 library headers
 *----------------------------------------------------------------------------*/

#if defined(HAVE_LIBXML2)

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#endif

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "bft_mem.h"
#include "bft_error.h"
#include "bft_printf.h"

#include "fvm_selector.h"

#include "mei_evaluate.h"

#include "cs_base.h"
#include "cs_gui_util.h"
#include "cs_gui_variables.h"
#include "cs_selector.h"
#include "cs_post.h"
#include "cs_field.h"
#include "cs_field_pointer.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_gui_output.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Local Macro Definitions
 *============================================================================*/

/* debugging switch */
#define _XML_DEBUG_ 0

/*============================================================================
 * Local Structure Definitions
 *============================================================================*/

/*============================================================================
 * External global variables
 *============================================================================*/

/*============================================================================
 * Private global variables
 *============================================================================*/

/*============================================================================
 * Static global variables
 *============================================================================*/

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Get output control value parameters.
 *
 * parameters:
 *   param               <--   name of the parameter
 *   keyword             -->   output control parameter
 *----------------------------------------------------------------------------*/

static void
_output_value(const char  *const param,
              int         *const keyword)
{
  char *path = NULL;
  int   result;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 3, "analysis_control", "output", param);

  if (cs_gui_strcmp(param, "auxiliary_restart_file_writing")) {

    cs_xpath_add_attribute(&path, "status");
    if(cs_gui_get_status(path, &result)) *keyword = result;

  }
  else {

    cs_xpath_add_function_text(&path);
    if (cs_gui_get_int(path, &result)) *keyword = result;

  }

  BFT_FREE(path);
}

/*----------------------------------------------------------------------------
 * Get output control value parameters for frequency output
 *
 * parameters:
 *   param               <--   name of the parameter
 *   keyword             -->   output control parameter
 *----------------------------------------------------------------------------*/

static void
_output_time_value(const char  *const param,
                   double      *const keyword)
{
  char *path = NULL;
  char *choice = NULL;
  double result = 0.0;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 3, "analysis_control", "output", param);

  cs_xpath_add_function_text(&path);
  if (cs_gui_get_double(path, &result)) *keyword = result;

  BFT_FREE(choice);
  BFT_FREE(path);
}

/*----------------------------------------------------------------------------
 * Return the output format and options for postprocessing.
 *
 * parameters
 *   param         -->  name of the parameter
 *   keyword       <--  output keyword string
 *   max_key_size  -->  maximum keyword size
 *----------------------------------------------------------------------------*/

static void
_output_choice(const char  *const param,
               char        *const keyword,
               size_t             max_key_size)
{
  char *path = NULL;
  char *choice = NULL;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 3, "analysis_control", "output", param);
  cs_xpath_add_attribute(&path, "choice");
  choice = cs_gui_get_attribute_value(path);
  BFT_FREE(path);

  if (choice != NULL) {
    strncpy(keyword, choice, max_key_size);
    keyword[max_key_size] = '\0';
  }
  else
    keyword[0] = '\0';

  BFT_FREE(choice);
}

/*----------------------------------------------------------------------------
 * Get postprocessing value status for surfacic variables
 *
 * parameters:
 *   name         --> name of the parameter
 *   default_val  --> default value
 *----------------------------------------------------------------------------*/

static bool
_surfacic_variable_post(const char  *name,
                        bool         default_val)
{
  int   result = 0;
  char *path = NULL;

  bool active = default_val;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "property");
  cs_xpath_add_test_attribute(&path, "name", name);

  if (cs_gui_get_nb_element(path) > 0) {

    /* If base path present but not recording status, default to true */
    active = true;

    cs_xpath_add_element(&path, "postprocessing_recording");
    cs_xpath_add_attribute(&path, "status");
    if (cs_gui_get_status(path, &result)) {
      if (result == 1)
        active = true;
      else
        active = false;
    }

  }

  BFT_FREE(path);

  return active;
}


/*----------------------------------------------------------------------------
 * Get the attribute value from the xpath query.
 *
 * parameters:
 *   path          <-- path for xpath query
 *   child         <-- child markup
 *   keyword       --> value of attribute node
 *----------------------------------------------------------------------------*/

static void
_attribute_value(char                *path,
                 const char    *const child,
                 int           *const keyword)
{
  int   result;

  assert(path != NULL);
  assert(child != NULL);

  cs_xpath_add_attribute(&path, "status");

  if (cs_gui_get_status(path, &result)) {
    *keyword = result;
  }
  else {

    if (cs_gui_strcmp(child, "postprocessing_recording") ||
        cs_gui_strcmp(child, "listing_printing")) *keyword = 1;
  }

  BFT_FREE(path);
}

/*----------------------------------------------------------------------------
 * Get the attribute value associated to a child markup from a variable.
 *
 * parameters:
 *   name          <--  name of the variable markup
 *   child         <--  child markup
 *   keyword       -->  value of attribute node contained in the child markup
 *----------------------------------------------------------------------------*/

static void
cs_gui_variable_attribute(const char *const name,
                          const char *const child,
                          int        *const keyword)
{
  char *path = NULL;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "variable");
  cs_xpath_add_test_attribute(&path, "name", name);
  cs_xpath_add_element(&path, child);
  _attribute_value(path, child, keyword);
}

/*-----------------------------------------------------------------------------
 * Return number of <probe recording> tags in the <variable> tag.
 *----------------------------------------------------------------------------*/

static int cs_gui_variable_number_probes(const char *const variable)
{
  char *path = NULL;
  char *choice = NULL;
  int   nb_probes;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "variable");
  cs_xpath_add_test_attribute(&path, "name", variable);
  cs_xpath_add_element(&path, "probes");
  cs_xpath_add_attribute(&path, "choice");
  choice = cs_gui_get_attribute_value(path);

  if (choice)
    nb_probes = atoi(choice);
  else
    nb_probes = -1;

  BFT_FREE(choice);
  BFT_FREE(path);

  return nb_probes;
}

/*-----------------------------------------------------------------------------
 * Return probe number for <probe_recording> tag for variable.
 *
 * parameters:
 *   variable   <--  name of variable
 *   num_probe  <--  number of <probe_recording> tags
 *----------------------------------------------------------------------------*/

static int cs_gui_variable_probe_name(const char *const variable,
                                      int               num_probe)
{
  char *path = NULL;
  char *strvalue = NULL;
  int   intvalue;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "variable");
  cs_xpath_add_test_attribute(&path, "name", variable);
  cs_xpath_add_element(&path, "probes");
  cs_xpath_add_element_num(&path, "probe_recording", num_probe);
  cs_xpath_add_attribute(&path, "name");

  strvalue = cs_gui_get_attribute_value(path);

  if (strvalue == NULL)
    bft_error(__FILE__, __LINE__, 0, _("Invalid xpath: %s\n"), path);

  intvalue = atoi(strvalue);

  BFT_FREE(strvalue);
  BFT_FREE(path);

  return intvalue;
}

/*-----------------------------------------------------------------------------
 * Return variable label.
 *
 * parameters:
 *   variable   <-- name of variable
 *----------------------------------------------------------------------------*/

static char *cs_gui_variable_label(const char *const variable)
{
  char *path = NULL;
  char *label = NULL;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "variable");
  cs_xpath_add_test_attribute(&path, "name", variable);
  cs_xpath_add_attribute(&path, "label");

  label = cs_gui_get_attribute_value(path);

  BFT_FREE(path);

  return label;
}

/*-----------------------------------------------------------------------------
 * Copy a variable name to private variable names array.
 *
 * parameters:
 *   varname        <--  name or label of the variable/scalar/property
 *   ipp            <--  index from the fortran array associated to varname
 *----------------------------------------------------------------------------*/

static void
_gui_copy_varname(const char *varname, int ipp)
{
  int i;
  size_t l;

  if (varname == NULL)
    return;

  /* Resize array if necessary */

  if (ipp > cs_glob_label->_cs_gui_max_vars) {

    if (cs_glob_label->_cs_gui_max_vars == 0)
      cs_glob_label->_cs_gui_max_vars = 16;

    while (cs_glob_label->_cs_gui_max_vars <= ipp)
      cs_glob_label->_cs_gui_max_vars *= 2;

    BFT_REALLOC(cs_glob_label->_cs_gui_var_name,
                cs_glob_label->_cs_gui_max_vars,
                char *);
    for (i = cs_glob_label->_cs_gui_last_var;
         i < cs_glob_label->_cs_gui_max_vars;
         i++)
      cs_glob_label->_cs_gui_var_name[i] = NULL;
  }

  if (ipp < 2 || ipp > cs_glob_label->_cs_gui_max_vars)
    bft_error(__FILE__, __LINE__, 0,
              _("Variable index %d out of bounds (1 to %d)"),
              ipp, cs_glob_label->_cs_gui_last_var);

  l = strlen(varname);

  if (cs_glob_label->_cs_gui_var_name[ipp-1] == NULL)
    BFT_MALLOC(cs_glob_label->_cs_gui_var_name[ipp-1], l + 1, char);

  else if (strlen(cs_glob_label->_cs_gui_var_name[ipp-1]) != l)
    BFT_REALLOC(cs_glob_label->_cs_gui_var_name[ipp-1], l + 1, char);

  strcpy(cs_glob_label->_cs_gui_var_name[ipp-1], varname);
}

/*-----------------------------------------------------------------------------
 * Post-processing options for all variables (velocity, pressure, ...)
 *
 * parameters:
 *   f_id               <-- field id
 *   ihisvr             --> histo output
 *   nvppmx             <-- number of printed variables
 *----------------------------------------------------------------------------*/

static void
_gui_variable_post(int         f_id,
                   int        *ihisvr,
                   const int  *nvppmx)
{
  int   nb_probes;
  int   iprob;
  char *varname = NULL;
  int   num_probe;

  const cs_field_t  *fi = cs_field_by_id(f_id);
  const int var_key_id = cs_field_key_id("post_id");
  int ipp = cs_field_get_key_int(fi, var_key_id);

  if (ipp == -1) return;

  int f_post = 0, f_log = 0;
  const int k_post = cs_field_key_id("post_vis");
  const int k_log  = cs_field_key_id("log");

  cs_gui_variable_attribute(fi->name,
                            "postprocessing_recording",
                            &f_post);

  cs_gui_variable_attribute(fi->name,
                            "listing_printing",
                            &f_log);

  cs_field_set_key_int(fi, k_post, f_post);
  cs_field_set_key_int(fi, k_log, f_log);

  nb_probes = cs_gui_variable_number_probes(fi->name);

  ihisvr[0 + (ipp - 1)] = nb_probes;

  if (fi->dim > 1)
    for (int idim = 1; idim < fi->dim; idim++)
      ihisvr[0 + (ipp - 1) + idim] = nb_probes;

  if (nb_probes > 0) {
    for (iprob = 0; iprob < nb_probes; iprob++) {
      num_probe = cs_gui_variable_probe_name(fi->name, iprob+1);
      ihisvr[(iprob+1)*(*nvppmx) + (ipp - 1)] = num_probe;
      if (fi->dim > 1)
        for (int idim = 1; idim < fi->dim; idim++)
          ihisvr[(iprob+1)*(*nvppmx) + (ipp - 1) + idim] = num_probe;
    }
  }

  varname = cs_gui_variable_label(fi->name);
  _gui_copy_varname(varname, ipp);

  BFT_FREE(varname);
}

/*-----------------------------------------------------------------------------
 * Return status of the property for physical model
 *
 * parameters:
 *   name          <--  name of the variable markup
 *   child         <--  child markup
 *   value_type    <-- type of value (listing_printing, postprocessing ..)
 *----------------------------------------------------------------------------*/

static void
cs_gui_property_output_status(const char *const name,
                              const char *const child,
                              int        *const keyword)
{
  char *path = NULL;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "property");
  cs_xpath_add_test_attribute(&path, "name", name);
  cs_xpath_add_element(&path, child);
  _attribute_value(path, child, keyword);
}

/*-----------------------------------------------------------------------------
 * Return probe number for sub balise "probe_recording" for property.
 *
 * parameters:
 *   property   <--  name of property
 *   num_probe  <-- number of <probe_recording> tags
 *----------------------------------------------------------------------------*/

static int cs_gui_property_probe_name(const char *const property,
                                      const int   num_probe)
{
  char *path = NULL;
  char *strvalue = NULL;
  int   value;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "property");
  cs_xpath_add_test_attribute(&path, "name", property);
  cs_xpath_add_element(&path, "probes");
  cs_xpath_add_element_num(&path, "probe_recording", num_probe);
  cs_xpath_add_attribute(&path, "name");

  strvalue = cs_gui_get_attribute_value(path);

  if (strvalue == NULL)
    bft_error(__FILE__, __LINE__, 0, _("Invalid xpath: %s\n"), path);

  value = atoi(strvalue);

  BFT_FREE(path);
  BFT_FREE(strvalue);

  return value;
}

/*-----------------------------------------------------------------------------
 * Return number of sub balises <probe_recording> for property.
 *----------------------------------------------------------------------------*/

static int cs_gui_property_number_probes(const char *const property)
{
  char *path = NULL;
  char *choice = NULL;
  int   nb_probes;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "property");
  cs_xpath_add_test_attribute(&path, "name", property);
  cs_xpath_add_element(&path, "probes");
  cs_xpath_add_attribute(&path, "choice");
  choice = cs_gui_get_attribute_value(path);

  if (choice) {
    nb_probes = atoi(choice);
    BFT_FREE(choice);
  }
  else
    nb_probes = -1;

  BFT_FREE(path);

  return nb_probes;
}

/*-----------------------------------------------------------------------------
 * Return the label model's property.
 *
 * parameters:
 *   property   <--  name of property
 *----------------------------------------------------------------------------*/

static char *cs_gui_get_property_label(const char *const property)
{
  char *path = NULL;
  char *label_name = NULL;

  path = cs_xpath_short_path();
  cs_xpath_add_element(&path, "property");
  cs_xpath_add_test_attribute(&path, "name", property);
  cs_xpath_add_attribute(&path, "label");

  label_name = cs_gui_get_attribute_value(path);

  BFT_FREE(path);

  return label_name;
}


/*-----------------------------------------------------------------------------
 * Post-processing options for properties
 *
 * parameters:
 *   f_id               <-- field id
 *   ihisvr             --> histo output
 *   nvppmx             <-- number of printed variables
 *----------------------------------------------------------------------------*/

static void
_gui_property_post(int         f_id,
                   int        *ihisvr,
                   const int  *nvppmx)
{
  int nb_probes;
  int iprob;
  char *varname = NULL;
  int num_probe;

  const cs_field_t  *fi = cs_field_by_id(f_id);
  const int var_key_id = cs_field_key_id("post_id");
  int ipp = cs_field_get_key_int(fi, var_key_id);

  if (ipp == -1) return;

  int f_post = 0, f_log = 0;
  const int k_post = cs_field_key_id("post_vis");
  const int k_log  = cs_field_key_id("log");

  /* EnSight outputs frequency */
  cs_gui_property_output_status(fi->name,
                                "postprocessing_recording",
                                &f_post);

  /* Listing output frequency */
  cs_gui_property_output_status(fi->name,
                                "listing_printing",
                                &f_log);

  cs_field_set_key_int(fi, k_post, f_post);
  cs_field_set_key_int(fi, k_log, f_log);

  /* Activated probes */
  nb_probes = cs_gui_property_number_probes(fi->name);

  ihisvr[0 + (ipp - 1)] = nb_probes;

  if (nb_probes > 0) {
    for (iprob=0; iprob<nb_probes; iprob++){
      num_probe = cs_gui_property_probe_name(fi->name, iprob+1);
      ihisvr[(iprob + 1)*(*nvppmx) + (ipp - 1)] = num_probe;
    }
  }

  /* Take into account labels */

  varname = cs_gui_get_property_label(fi->name);
  _gui_copy_varname(varname, ipp);

  BFT_FREE(varname);
}


/*-----------------------------------------------------------------------------
 * Return a single coordinate of a monitoring probe.
 *
 * parameters
 *   num_probe            <--  number aka name of the monitoring probe
 *   probe_coord          <--  one coordinate of the monitoring probe
 *----------------------------------------------------------------------------*/

static double cs_gui_probe_coordinate(const int         num_probe,
                                      const char *const probe_coord)
{
  char  *path = NULL;
  double result = 0.0;

  assert(num_probe > 0);

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, "probe", num_probe);
  cs_xpath_add_element(&path, probe_coord);
  cs_xpath_add_function_text(&path);

  if (!cs_gui_get_double(path, &result))
    bft_error(__FILE__, __LINE__, 0,
              _("Coordinate %s of the monitoring probe number %i "
                "not found.\nXpath: %s\n"), probe_coord, num_probe, path);

  BFT_FREE(path);

  return result;
}

/*----------------------------------------------------------------------------
 * Return the location of a mesh.
 *
 * parameters:
 *   num                <--  number of a mesh
 *----------------------------------------------------------------------------*/

static char *cs_gui_output_mesh_location(int  const num)
{
  char *path = NULL;
  char *location = NULL;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, "mesh", num);
  cs_xpath_add_element(&path, "location");
  cs_xpath_add_function_text(&path);
  location = cs_gui_get_text_value(path);

  BFT_FREE(path);
  return location;
}

/*----------------------------------------------------------------------------
 * Return the density for a particle mesh.
 *
 * parameters:
 *   num                <--  number of a mesh
 *----------------------------------------------------------------------------*/

static double cs_gui_output_particle_density(int  const num)
{
  char *path = NULL;
  double result = 1.;

  double density = 1.;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, "mesh", num);
  cs_xpath_add_element(&path, "density");
  cs_xpath_add_function_text(&path);
  if (cs_gui_get_double(path, &result))
    density = result;

  BFT_FREE(path);
  return density;
}

/*----------------------------------------------------------------------------
 * Return the frequency of a writer.
 *
 * parameters:
 *   num                <--  number of the writer
 *----------------------------------------------------------------------------*/

static double cs_gui_output_writer_frequency(int  const num)
{
  char *path = NULL;
  char *path_bis = NULL;
  double time_step = 0.0;
  double result = 0.0;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, "writer", num);
  BFT_MALLOC(path_bis, strlen(path) +1, char);
  strcpy(path_bis, path);
  cs_xpath_add_element(&path, "frequency");
  cs_xpath_add_function_text(&path);
  if (cs_gui_get_double(path, &result))
    time_step = result;
  else {
    cs_xpath_add_element(&path_bis, "frequency_time");
    cs_xpath_add_function_text(&path_bis);
    if (cs_gui_get_double(path_bis, &result))
      time_step = result;
  }

  BFT_FREE(path);
  BFT_FREE(path_bis);
  return time_step;
}

/*----------------------------------------------------------------------------
 * Return an option for a mesh or a writer call by a number.
 *
 * parameters:
 *   type                 <--  'writer' or 'mesh'
 *   choice               <--  type of option to get
 *   option               <--  the option needed
 *   num                  <--  number of the mesh or the writer
 *----------------------------------------------------------------------------*/

static char *cs_gui_output_type_options(const char *const type,
                                        const char *const choice,
                                        const char *const option,
                                        int         const num)
{
  char *path = NULL;
  char *description = NULL;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, type, num);
  cs_xpath_add_element(&path, option);

  if (cs_gui_strcmp(option, "frequency") && choice == NULL) {
    description = cs_gui_get_text_value(path);
  }
  else {
    cs_xpath_add_attribute(&path, choice);
    description = cs_gui_get_attribute_value(path);
  }

  BFT_FREE(path);

  if (description == NULL) {
    BFT_MALLOC(description, 1, char);
    description[0] = '\0';
  }

  return description;
}

/*----------------------------------------------------------------------------
 * Return the id of a writer associated with a mesh.
 *
 * parameters:
 *   num_mesh                <--  number of the given mesh
 *   num_writer              <--  number of the associated writer
 *----------------------------------------------------------------------------*/

static int cs_gui_output_associate_mesh_writer(int  const num_mesh,
                                               int  const num_writer)
{
  char *path = NULL;
  char *id = NULL;
  int description;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, "mesh"  , num_mesh);
  cs_xpath_add_element_num(&path, "writer", num_writer);
  cs_xpath_add_attribute(&path, "id");

  id = cs_gui_get_attribute_value(path);
  description = atoi(id);

  BFT_FREE(path);
  BFT_FREE(id);
  return description;
}

/*----------------------------------------------------------------------------
 * Return a choice for a mesh or a writer call by a number.
 *
 * parameters:
 *   type                <--   'writer' or 'mesh'
 *   choice              <--   the option needed
 *   num                 <--   the number of the mesh or writer
 *----------------------------------------------------------------------------*/

static char *cs_gui_output_type_choice(const char *const type,
                                       const char *const choice,
                                       int         const num)
{
  char *path = NULL;
  char *description = NULL;

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, type, num);
  cs_xpath_add_attribute(&path, choice);
  description = cs_gui_get_attribute_value(path);

  BFT_FREE(path);
  return description;
}

/*-----------------------------------------------------------------------------
 * Initialize mei tree and check for symbols existence.
 *
 * parameters:
 *   formula        <--  mei formula
 *   symbols        <--  array of symbol to check
 *   symbol_size    -->  number of symbol in symbols
 *----------------------------------------------------------------------------*/

static mei_tree_t *_init_mei_tree(const int        num,
                                  const cs_int_t  *ntcabs,
                                  const cs_real_t *ttcabs)
{
  char *path = NULL;
  char *formula = NULL;

  /* return an empty interpreter */

  path = cs_xpath_init_path();
  cs_xpath_add_elements(&path, 2, "analysis_control", "output");
  cs_xpath_add_element_num(&path, "writer", num);
  cs_xpath_add_element(&path, "frequency");
  cs_xpath_add_function_text(&path);
  formula = cs_gui_get_text_value(path);
  mei_tree_t *tree = mei_tree_new(formula);

  /* add commun variables */
  mei_tree_insert(tree, "niter", *ntcabs );
  mei_tree_insert(tree, "t", *ttcabs);

  /* try to build the interpreter */
  if (mei_tree_builder(tree))
    bft_error(__FILE__, __LINE__, 0,
              _("Error: can not interpret expression: %s\n"), tree->string);
  /* check for symbols */
  if (mei_tree_find_symbol(tree, "iactive"))
    bft_error(__FILE__, __LINE__, 0,
              _("Error: can not find the required symbol: %s\n"), "iactive");

  return tree;
}

/*----------------------------------------------------------------------------
 * Copy variable labels from fields to GUI labels
 *----------------------------------------------------------------------------*/

static void
_field_labels_to_gui(void)
{
  int i;

  const int n_fields = cs_field_n_fields();
  const int k_post = cs_field_key_id("post_id");

  for (int f_id = 0; f_id < n_fields; f_id++) {

    const cs_field_t *f = cs_field_by_id(f_id);
    const int ipp = cs_field_get_key_int(f, k_post);

    if (ipp < 2)
      continue;

    /* Loop on field components */

    const int p_dim = CS_MIN(f->dim, 3);

    for (int c_id = 0; c_id < p_dim; c_id++) {

      int p_id = ipp - 1 + c_id;
      char label[128];

      /* Resize array if necessary */

      if (p_id >= cs_glob_label->_cs_gui_max_vars) {

        if (cs_glob_label->_cs_gui_max_vars == 0)
          cs_glob_label->_cs_gui_max_vars = 16;
        while (cs_glob_label->_cs_gui_max_vars <= p_id)
          cs_glob_label->_cs_gui_max_vars *= 2;

        BFT_REALLOC(cs_glob_label->_cs_gui_var_name,
                    cs_glob_label->_cs_gui_max_vars,
                    char *);
        for (i = cs_glob_label->_cs_gui_last_var;
             i < cs_glob_label->_cs_gui_max_vars;
             i++)
          cs_glob_label->_cs_gui_var_name[i] = NULL;

      }

      /* Build name */

      const char *f_label = cs_field_get_label(f);

      if (f->dim == 1)
        strncpy(label, f_label, 127);
      else if (f->dim == 3)
        snprintf(label, 127, "%s%s", f_label, cs_glob_field_comp_name_3[c_id]);
      else if (f->dim == 6)
        snprintf(label, 127, "%s%s", f_label, cs_glob_field_comp_name_6[c_id]);
      else if (f->dim == 9)
        snprintf(label, 127, "%s%s", f_label, cs_glob_field_comp_name_9[c_id]);
      else
        snprintf(label, 127, "%s[%02d]", f_label, c_id);
      label[127] = '\0';


      BFT_REALLOC(cs_glob_label->_cs_gui_var_name[p_id],
                  strlen(label) + 1,
                  char);

      strcpy(cs_glob_label->_cs_gui_var_name[p_id], label);

      /* Update variable counter */

      if (p_id + 1 > cs_glob_label->_cs_gui_last_var)
        cs_glob_label->_cs_gui_last_var = p_id + 1;

    }

  }

}

/*----------------------------------------------------------------------------
 * Copy variable labels to fields from GUI labels
 *----------------------------------------------------------------------------*/

static void
_field_labels_from_gui(void)
{
  int i;

  const int n_fields = cs_field_n_fields();
  const int k_post = cs_field_key_id("post_id");
  const int k_lbl = cs_field_key_id("label");

  for (int f_id = 0; f_id < n_fields; f_id++) {

    cs_field_t *f = cs_field_by_id(f_id);
    const int ipp = cs_field_get_key_int(f, k_post);

    if (ipp < 2)
      continue;

    /* Initial checks and label setup for a given field */

    const int p_dim = CS_MIN(f->dim, 3);

    int p_id = ipp - 1;
    char label[128];

    if (p_id > cs_glob_label->_cs_gui_max_vars)
      continue;

    if (cs_glob_label->_cs_gui_var_name[p_id] == NULL)
      continue;

    strcpy(label, cs_glob_label->_cs_gui_var_name[p_id]);

    /* In case of multiple field components, keep common part */

    for (int c_id = 1; c_id < p_dim; c_id++) {

      p_id += 1;

      if (p_id > cs_glob_label->_cs_gui_max_vars)
        continue;
      if (cs_glob_label->_cs_gui_var_name[p_id] == NULL)
        continue;

      for (i = 0;
           (   label[i] == cs_glob_label->_cs_gui_var_name[p_id][i]
            && label[i] != '\0'
            && label[i] != '[');
           i++);

      if (i > 0)
        label[i] = '\0';

    }

    /* Update field label */

    cs_field_set_key_str(f, k_lbl, label);

  }
}

/*----------------------------------------------------------------------------
 * Clean memory for fortran name of variables
 *----------------------------------------------------------------------------*/

static void
_gui_free_labels(void)
{
  int i;
#if _XML_DEBUG_
  bft_printf("%s\n", __funct__);
  for (i = 0; i < cs_glob_label->_cs_gui_max_vars; i++)
    if (cs_glob_label->_cs_gui_var_name[i])
      bft_printf("-->label[%i] = %s\n", i, cs_glob_label->_cs_gui_var_name[i]);
#endif

  for (i = 0; i < cs_glob_label->_cs_gui_max_vars; i++)
    BFT_FREE(cs_glob_label->_cs_gui_var_name[i]);

  BFT_FREE(cs_glob_label->_cs_gui_var_name);

  cs_glob_label->_cs_gui_max_vars = 0;
  cs_glob_label->_cs_gui_last_var = 0;
}

/*============================================================================
 * Public Fortran function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * activation of a writer depending of a formula
 *
 * Fortran Interface:
 *
 * subroutine uinpst (ttcabs, ntcabs)
 * *****************
 *
 * integer         uref   -->   reference velocity
 * double          almax  -->   reference length
 *----------------------------------------------------------------------------*/

void CS_PROCF (uinpst, UINPST) (const cs_int_t  *ntcabs,
                                const cs_real_t *ttcabs)
{
  int i, id, nwriter;
  int iactive;
  char *frequency_choice;
  char *id_s;
  mei_tree_t *ev_formula  = NULL;
  nwriter = cs_gui_get_tag_number("/analysis_control/output/writer", 1);
  for (i = 1; i <= nwriter; i++) {
    id = 0;
    id_s = cs_gui_output_type_choice("writer","id",i);
    if (id_s != NULL) {
      id = atoi(id_s);
      BFT_FREE(id_s);
    }
    frequency_choice
      = cs_gui_output_type_options("writer", "period", "frequency", i);
    if (cs_gui_strcmp(frequency_choice, "formula")) {
      ev_formula = _init_mei_tree(i, ntcabs, ttcabs);
      mei_evaluate(ev_formula);
      iactive =  mei_tree_lookup(ev_formula, "iactive");
      mei_tree_destroy(ev_formula);
      if (iactive == 1)
        cs_post_activate_writer(id, true);
      else
        cs_post_activate_writer(id, false);
    }
    BFT_FREE(frequency_choice);
  }
}

/*----------------------------------------------------------------------------
 * Determine output options.
 *----------------------------------------------------------------------------*/

void CS_PROCF (csenso, CSENSO) (const cs_int_t  *nvppmx,
                                cs_int_t        *ncapt,
                                cs_int_t        *nthist,
                                cs_real_t       *frhist,
                                cs_int_t        *ntlist,
                                cs_int_t        *iecaux,
                                cs_int_t        *ipstdv,
                                cs_int_t        *ihisvr,
                                cs_int_t        *tplfmt,
                                const cs_int_t  *isca,
                                const cs_int_t  *iscapp,
                                const cs_int_t  *ipprtp,
                                cs_real_t       *xyzcap)
{
  int i, j;
  int ipp;
  int *ippfld;
  cs_var_t  *vars = cs_glob_var;
  char fmtprb[16];

  {
    int n_fields = cs_field_n_fields();
    int ipp_max = 1;
    const int k_pp = cs_field_key_id("post_id");
    for (int f_id = 0; f_id < n_fields; f_id++) {
      const cs_field_t *f = cs_field_by_id(f_id);
      int ipp1 = cs_field_get_key_int(f, k_pp);
      int ipp2 = ipp1 + f->dim - 1;
      ipp_max = CS_MAX(ipp_max, ipp2);
    }
    BFT_MALLOC(ippfld, ipp_max, int);
    for (ipp = 0; ipp < ipp_max; ipp++)
      ippfld[ipp] = -1;
    for (int f_id = 0; f_id < n_fields; f_id++) {
      const cs_field_t *f = cs_field_by_id(f_id);
      ipp = cs_field_get_key_int(f, k_pp);
      if (ipp > 1)
        ippfld[ipp - 1] = f_id;
    }
  }

  _field_labels_to_gui();

  _output_value("auxiliary_restart_file_writing", iecaux);
  _output_value("listing_printing_frequency", ntlist);
  _output_value("probe_recording_frequency", nthist);
  _output_time_value("probe_recording_frequency_time", frhist);

  _output_choice("probe_format", fmtprb, sizeof(fmtprb) - 1);

  /* Time plot (probe) format */
  if (!strcmp(fmtprb, "DAT"))
    *tplfmt = 1;
  else if (!strcmp(fmtprb, "CSV"))
    *tplfmt = 2;

  /* Surfacic variables output */

  for (i = 0; i < 6; i++)
    ipstdv[i] = 0;

  if (_surfacic_variable_post("effort", true))
    ipstdv[0] += 1;
  if (_surfacic_variable_post("effort_tangential", false))
    ipstdv[0] += 2;
  if (_surfacic_variable_post("effort_normal", false))
    ipstdv[0] += 4;

  if (_surfacic_variable_post("yplus", true))
    ipstdv[1] = 1;
  if (_surfacic_variable_post("tplus", true))
    ipstdv[2] = 1;
  if (_surfacic_variable_post("thermal_flux", true))
    ipstdv[3] = 1;
  if (_surfacic_variable_post("boundary_temperature", true))
    ipstdv[4] = 1;
  if (_surfacic_variable_post("boundary_layer_nusselt", true))
    ipstdv[5] = 1;

  *ncapt = cs_gui_get_tag_number("/analysis_control/output/probe", 1);
  for (i = 0; i < *ncapt; i++) {
    xyzcap[0 + i*3] = cs_gui_probe_coordinate(i+1, "probe_x");
    xyzcap[1 + i*3] = cs_gui_probe_coordinate(i+1, "probe_y");
    xyzcap[2 + i*3] = cs_gui_probe_coordinate(i+1, "probe_z");
  }

  /* variable output */
  int n_fields = cs_field_n_fields();
  for (int f_id = 0; f_id < n_fields; f_id++) {
    const cs_field_t  *f = cs_field_by_id(f_id);
    if (f->type & CS_FIELD_VARIABLE)
      _gui_variable_post(f->id, ihisvr, nvppmx);
    else if (f->type & CS_FIELD_PROPERTY)
      _gui_property_post(f->id, ihisvr, nvppmx);
  }

  _field_labels_from_gui();

#if _XML_DEBUG_
  bft_printf("==>CSENSO\n");
  bft_printf("--iecaux = %i\n", *iecaux);
  bft_printf("--ntlist = %i\n", *ntlist);
  bft_printf("--nthist = %i\n", *nthist);
  bft_printf("--frhist = %f\n", *frhist);
  bft_printf("--ncapt  = %i\n", *ncapt);
  bft_printf("--tplfmt = %i\n", *tplfmt);
  for (i=0; i < *ncapt; i++) {
    bft_printf("--xyzcap[%i][0] = %f\n", i, xyzcap[0 +i*3]);
    bft_printf("--xyzcap[%i][1] = %f\n", i, xyzcap[1 +i*3]);
    bft_printf("--xyzcap[%i][2] = %f\n", i, xyzcap[2 +i*3]);
  }
  const int k_post = cs_field_key_id("post_id");
  for (int f_id = 0; f_id < cs_field_n_fields(); f_id++) {
    const cs_field_t *f = cs_field_by_id(f_id);
    const int ipp = cs_field_get_key_int(f, k_post);
    bft_printf("-->variable ipprtp[%i] = %s\n", ipp, f->name);
    bft_printf("--ihisvr[0][%i]= %i \n", ipp, ihisvr[0 + (ipp-1)]);
    if (ihisvr[0 + (ipp-1)]>0)
      for (j=0; j<ihisvr[0 + (ipp-1)]; j++)
        bft_printf("--ihisvr[%i][%i]= %i \n", j+1, ipp,
                   ihisvr[(j+1)*(*nvppmx) + (ipp-1)]);
  }
#endif

  BFT_FREE(ippfld);

  _gui_free_labels();
}

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Define postprocessing meshes using an XML file.
 *----------------------------------------------------------------------------*/

void
cs_gui_postprocess_meshes(void)
{
  int i, j, id, id_writer;
  char *id_s = NULL;
  char *label = NULL;
  char *all_variables = NULL;
  bool auto_vars = true;
  bool add_groups = true;
  char *location = NULL;
  int n_writers, nmesh;
  char *type = NULL;
  char *path = NULL;
  int *writer_ids = NULL;

  if (!cs_gui_file_is_loaded())
    return;

  nmesh = cs_gui_get_tag_number("/analysis_control/output/mesh", 1);

  for (i = 1; i <= nmesh; i++) {
    id_s = cs_gui_output_type_choice("mesh","id",i);
    id = atoi(id_s);
    label = cs_gui_output_type_choice("mesh","label",i);
    all_variables
      = cs_gui_output_type_options("mesh", "status", "all_variables",i);
    if (cs_gui_strcmp(all_variables,"on"))
      auto_vars = true;
    else if (cs_gui_strcmp(all_variables, "off"))
      auto_vars = false;
    location = cs_gui_output_mesh_location(i);
    type = cs_gui_output_type_choice("mesh","type",i);
    path = cs_xpath_init_path();
    cs_xpath_add_elements(&path, 2, "analysis_control", "output");
    cs_xpath_add_element_num(&path, "mesh", i);
    cs_xpath_add_element(&path, "writer");
    n_writers = cs_gui_get_nb_element(path);
    BFT_MALLOC(writer_ids, n_writers, int);
    for (j = 0; j <= n_writers-1; j++) {
      id_writer = cs_gui_output_associate_mesh_writer(i,j+1);
      writer_ids[j] = id_writer;
    }
    if (cs_gui_strcmp(type, "cells")) {
      cs_post_define_volume_mesh(id, label, location, add_groups, auto_vars,
                                 n_writers, writer_ids);
    } else if(cs_gui_strcmp(type, "interior_faces")) {
      cs_post_define_surface_mesh(id, label, location, NULL,
                                  add_groups, auto_vars,
                                  n_writers, writer_ids);
    } else if(cs_gui_strcmp(type, "boundary_faces")) {
      cs_post_define_surface_mesh(id, label, NULL, location,
                                  add_groups, auto_vars,
                                  n_writers, writer_ids);
    } else if(cs_gui_strcmp(type, "boundary_faces")) {
      cs_post_define_surface_mesh(id, label, NULL, location,
                                  add_groups, auto_vars,
                                  n_writers, writer_ids);
    } else if(   cs_gui_strcmp(type, "particles")
              || cs_gui_strcmp(type, "trajectories")) {
      bool trajectory = cs_gui_strcmp(type, "trajectories") ? true : false;
      double density = cs_gui_output_particle_density(i);
      cs_post_define_particles_mesh(id, label, location,
                                    density, trajectory, auto_vars,
                                    n_writers, writer_ids);
    }

    BFT_FREE(writer_ids);
    BFT_FREE(id_s);
    BFT_FREE(label);
    BFT_FREE(all_variables);
    BFT_FREE(location);
    BFT_FREE(type);
    BFT_FREE(path);
  }
}

/*----------------------------------------------------------------------------
 * Define postprocessing writers using an XML file.
 *----------------------------------------------------------------------------*/

void
cs_gui_postprocess_writers(void)
{
  int i;
  char *label = NULL;
  char *directory = NULL;
  char *format_name = NULL;
  char *format_options = NULL;
  char *time_dependency = NULL;
  char *frequency_choice = NULL;
  char *output_end_s = NULL;
  char *id_s = NULL;

  int n_writers = 0;

  if (!cs_gui_file_is_loaded())
    return;

  n_writers = cs_gui_get_tag_number("/analysis_control/output/writer", 1);

  for (i = 1; i <= n_writers; i++) {

    int id = 0;
    fvm_writer_time_dep_t  time_dep = FVM_WRITER_FIXED_MESH;
    bool output_at_end = true;
    cs_int_t time_step = -1;
    cs_real_t time_value = -1.0;

    id_s = cs_gui_output_type_choice("writer", "id", i);
    id = atoi(id_s);
    label = cs_gui_output_type_choice("writer", "label", i);
    directory = cs_gui_output_type_options("writer", "name", "directory", i);
    frequency_choice
      = cs_gui_output_type_options("writer", "period", "frequency", i);
    output_end_s
      = cs_gui_output_type_options("writer", "status", "output_at_end", i);
    if (cs_gui_strcmp(frequency_choice, "none")) {
      time_step = -1;
      time_value = -1.;
    } else if (cs_gui_strcmp(frequency_choice, "time_step")) {
      time_step = (int)cs_gui_output_writer_frequency(i);
      time_value = -1.;
    } else if (cs_gui_strcmp(frequency_choice, "time_value")) {
      time_step = -1;
      time_value = cs_gui_output_writer_frequency(i);
    } else if (cs_gui_strcmp(frequency_choice, "formula")) {
      time_step = -1;
      time_value = -1.;
    }
    if (cs_gui_strcmp(output_end_s, "off"))
      output_at_end = false;
    format_name = cs_gui_output_type_options("writer", "name", "format",i);
    format_options
      = cs_gui_output_type_options("writer", "options", "format", i);
    time_dependency
      = cs_gui_output_type_options("writer", "choice", "time_dependency", i);
    if (cs_gui_strcmp(time_dependency, "fixed_mesh"))
      time_dep = FVM_WRITER_FIXED_MESH;
    else if(cs_gui_strcmp(time_dependency, "transient_coordinates"))
      time_dep = FVM_WRITER_TRANSIENT_COORDS;
    else if(cs_gui_strcmp(time_dependency, "transient_connectivity"))
      time_dep = FVM_WRITER_TRANSIENT_CONNECT;
    cs_post_define_writer(id,
                          label,
                          directory,
                          format_name,
                          format_options,
                          time_dep,
                          output_at_end,
                          time_step,
                          time_value);
    BFT_FREE(id_s);
    BFT_FREE(label);
    BFT_FREE(format_name);
    BFT_FREE(format_options);
    BFT_FREE(time_dependency);
    BFT_FREE(output_end_s);
    BFT_FREE(frequency_choice);
    BFT_FREE(directory);
  }
}

/*-----------------------------------------------------------------------------
 * Post-processing options for fields
 *
 * These options are used for fields not mapped to variables or properties.
 *----------------------------------------------------------------------------*/

void
cs_gui_postprocess_fields(void)
{
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
