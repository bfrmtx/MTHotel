# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'MTHotel / MTH5'
copyright = '2024, Bernhard Friedrichs'
author = 'Bernhard Friedrichs'
release = '1.2'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration
# , "sphinx_rtd_theme" I use press here
extensions = ["myst_parser", "sphinx.ext.autosectionlabel"]

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
    "substitution",
]

myst_heading_anchors = 3
numfig = True
language = 'en'
html_show_sourcelink = False

source_suffix = {
    '.md': 'markdown',
}

templates_path = ['_templates']

# this is a redirection page to the "landing page"
html_additional_pages = {
    "index": "landing_page.html"
}

# do not scan the SQLITE dir and auto created md files
exclude_patterns = ['_build', 'build', 'Thumbs.db', '.DS_Store', 'sqltables', 'vscode']


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output
html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
# indent Python
def setup(app):
    app.add_css_file('css/100_theme.css')

html_theme_options = {
    'prev_next_buttons_location': 'bottom',
    'titles_only': False  # False so page subheadings are in the nav.
}


html_logo = '_static/images/metronix_Logo_geo_4c_09-2017_tiny.png'
html_favicon = '_static/images/metronix_Logo_geo_4c_09-2017_icon.png'


# I have also modified source/_templates/layout.html to get the index on the left (online) and (offline)
