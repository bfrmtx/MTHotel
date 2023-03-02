# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'MTHotel / MTH5'
copyright = '2023, Bernhard Friedrichs'
author = 'Bernhard Friedrichs'
release = '1.2'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration
# , "sphinx_rtd_theme" I use press here
extensions = ["myst_parser", "sphinx.ext.autosectionlabel", "sphinx_rtd_theme"]

myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "dollarmath",
    "amsmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
]

myst_heading_anchors = 3
numfig = True
language = 'en'

templates_path = ['_templates']

# this is a redirection page to the "landing page"
html_additional_pages = {
    "index": "landing_page.html"
}
exclude_patterns = ['_build', 'build', 'Thumbs.db', '.DS_Store', 'vscode', 'sqltables']


# Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'press'
html_static_path = ['_static']

html_logo = '_static/images/metronix_Logo_geo_4c_09-2017_tiny.png'
html_favicon = '_static/images/metronix_Logo_geo_4c_09-2017_icon.png'

