# FairMQ documentation site

This folder is the source of <https://fairrootgroup.github.io/FairMQ/>. The site
uses [Astro](https://astro.build/) with [Starlight](https://starlight.astro.build/).
The workflow [`docs.yml`](../.github/workflows/docs.yml) builds the site for each
pull request and deploys it to GitHub Pages for each push to `dev`.

## Requirements

* Node.js 22.23.4 or newer
* Doxygen 1.9 or newer. The API reference and the source browser use its XML output.
* git. The site shows the version from `git describe`.

## Build

```sh
cd docs
npm ci
npm run dev       # local server at http://localhost:4321/FairMQ/, it reloads after changes
npm run build     # writes the site to dist/
npm run check     # checks all links in dist/
npm run preview   # serves dist/
```

The build runs Doxygen on `fairmq/` and writes the XML to `build/doxygen/`. The next
builds run Doxygen again only when a file in `fairmq/` is newer than the XML.

## Contents

| Path | Contents |
| --- | --- |
| `src/content/docs/` | The pages, in MDX. Each folder is a group of the sidebar. |
| `src/pages/api/` | The API reference, from the Doxygen XML |
| `src/pages/source/` | The source browser, from the Doxygen XML |
| `src/pages/404.astro` | The 404 page. It sends the URLs of the old Doxygen site to the new pages. |
| `src/components/` | Components for the pages: `Snippet`, the diagrams, the option tables |
| `src/data/option-texts.mjs` | The descriptions of the command-line options. Names, types and defaults come from the code. |
| `scripts/` | The data for the generated pages, and the checks |

## Write a page

* Each page starts with frontmatter: `title`, `description` and `sidebar.order`.
* Show code of the repository with `<Snippet>`, not with a copy. Then the samples
  are code that CI compiles, and the build fails when the code changes and the
  sample does not agree. `src/components/Snippet.astro` shows the usage.
* Write links root-relative and without the base: `/concepts/channels/`,
  `/api/fair/mq/Channel/`, `/source/fairmq/Channel.h/#L36`.
* Run `node scripts/check-pages.mjs` before you build. It checks the frontmatter, the
  MDX syntax, the snippets and all links in a few seconds.
* Write in the style of ASD-STE100 (Simplified Technical English): short sentences,
  one instruction for each sentence, the active voice.
