# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import datetime

import geoarrow.c

sys.path.insert(0, os.path.abspath(".."))

# -- Project information -----------------------------------------------------

project = "geoarrow"
copyright = f"2016-{datetime.datetime.now().year} Dewey Dunnington"
author = "Dewey Dunnington"


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "breathe",
    "sphinx.ext.autodoc",
]

# Breathe configuration
breathe_projects = {
    "geoarrow_c": "../../src/apidoc/xml",
}
breathe_default_project = "geoarrow_c"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["README_generated.rst"]


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "pydata_sphinx_theme"

html_theme_options = {
    "show_toc_level": 2,
    "use_edit_page_button": True,
    "external_links": [],
}

html_context = {
    "github_user": "geoarrow",
    "github_repo": "geoarrow-c",
    "github_version": "main",
    "doc_path": "docs/source",
}

html_sidebars = {"**": ["search-field", "sidebar-nav-bs"]}


# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = []

add_module_names = False
