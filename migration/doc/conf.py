# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import subprocess
import sys

sys.path.insert(0, os.path.abspath(".") + "/_extensions")

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "Phlex Migration Guide"
copyright = "2026, Fermi Forward Discovery Group, LLC"
author = ""

git_commit = (
    subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
    .decode("ascii")
    .strip()
)

release = f"0.1.alpha ({git_commit})"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx.ext.autosectionlabel",
]

autosectionlabel_prefix_document = True
numfig = True
numfig_format = {
    "code-block": "Listing %s",
    "figure": "Fig. %s",
    "section": "Section %s",
    "table": "Table %s",
}


# -- Code-highlighting roles -------------------------------------------------

rst_prolog = """
.. role:: cpp(code)
   :language: c++
   :class: highlight

.. role:: py(code)
   :language: py
   :class: highlight
"""

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "bizstyle"
html_static_path = ["_static"]
html_style = "phlexframework.css"
html_last_updated_fmt = ""  # Equivalent to showing (e.g.) May 29, 2025

# -- Options for LaTeX output ------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-latex-output

latex_elements = {}

latex_documents = [
    ("index", "phlex-migration-guide.tex", project, author, "manual", False)
]
