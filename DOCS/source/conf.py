# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'AppGuard'
copyright = '2024, Oussama Ben Gatrane'
author = 'Oussama Ben Gatrane'
release = '1.01'
version = '1.01'

import os
import sys
# Corrected path: Add the parent directory of the 'app_guard' package
sys.path.insert(0, os.path.abspath('../..'))

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "myst_parser",
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'breathe',
    'sphinx_rtd_theme',
    'sphinx.ext.viewcode',
    'sphinx.ext.napoleon',
    'sphinx_sitemap',
]

# Breathe configuration for C++ API documentation
breathe_projects = { "AppGuard": "../doxygen_output/xml/" }
breathe_default_project = "AppGuard"

# Autodoc configuration
autodoc_default_options = {
    'members': True,
    'member-order': 'bysource',
    'special-members': '__init__',
    'undoc-members': True,
    'exclude-members': '__weakref__'
}

# Autosummary configuration
autosummary_generate = True

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# MyST parser settings
myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "tasklist",
    "attrs_inline",
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Theme options
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',
    'logo_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'vcs_pageview_mode': '',
    'style_nav_header_background': '#2980B9',
    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

html_title = f"{project} v{version}"
html_short_title = project
html_logo = None
html_favicon = None

# Custom CSS
html_css_files = [
    'custom.css',
]

# Sitemap configuration
html_baseurl = 'https://your-domain.com/' # Remember to change this to your actual domain
sitemap_url_scheme = "{link}"