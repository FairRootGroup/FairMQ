// @ts-check
import { readFileSync } from 'node:fs';
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import starlightLinksValidator from 'starlight-links-validator';
import { satteri } from '@astrojs/markdown-satteri';
import baseLinks from './scripts/base-links.mjs';
import nowrapCode from './scripts/nowrap-code.mjs';
import fairmqData from './scripts/integration.mjs';

const base = '/FairMQ';
const codeTheme = JSON.parse(readFileSync(new URL('./src/styles/code-theme.json', import.meta.url), 'utf8'));

export default defineConfig({
  site: 'https://fairrootgroup.github.io',
  base,
  markdown: {
    processor: satteri({ hastPlugins: [baseLinks({ base }), nowrapCode()] }),
    // Keep -- and ... as typed: options like --rate and quoted log output must stay exact.
    smartypants: false,
  },
  integrations: [
    fairmqData(),
    starlight({
      title: 'FairMQ',
      description: 'FairMQ is a C++17 message queuing library and framework for large-scale data processing.',
      favicon: '/favicon.svg',
      social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/FairRootGroup/FairMQ' }],
      editLink: { baseUrl: 'https://github.com/FairRootGroup/FairMQ/edit/dev/docs/' },
      lastUpdated: true,
      disable404Route: true,
      customCss: [
        '@fontsource-variable/archivo/wdth.css',
        '@fontsource-variable/jetbrains-mono/wght.css',
        '@fontsource-variable/jetbrains-mono/wght-italic.css',
        './src/styles/theme.css',
      ],
      components: {
        Header: './src/components/overrides/Header.astro',
        PageTitle: './src/components/overrides/PageTitle.astro',
        Footer: './src/components/overrides/Footer.astro',
        PageSidebar: './src/components/overrides/PageSidebar.astro',
        Hero: './src/components/overrides/Hero.astro',
        MobileMenuFooter: './src/components/overrides/MobileMenuFooter.astro',
      },
      expressiveCode: {
        themes: [codeTheme],
        useStarlightDarkModeSwitch: false,
        useStarlightUiThemeColors: false,
        styleOverrides: {
          borderRadius: '6px',
          borderColor: 'var(--fmq-code-rule)',
          codeBackground: 'var(--fmq-code-bg)',
          codeFontFamily: 'var(--fmq-font-mono)',
          codeFontSize: '0.84rem',
          codeLineHeight: '1.65',
          uiFontFamily: 'var(--fmq-font-body)',
          frames: {
            editorBackground: 'var(--fmq-code-bg)',
            editorTabBarBackground: 'var(--fmq-code-bg)',
            editorActiveTabBackground: 'var(--fmq-code-bg)',
            editorActiveTabIndicatorTopColor: 'transparent',
            editorActiveTabIndicatorBottomColor: 'var(--fmq-saffron)',
            editorTabBarBorderBottomColor: 'var(--fmq-code-rule)',
            terminalBackground: 'var(--fmq-code-bg)',
            terminalTitlebarBackground: 'var(--fmq-code-bg)',
            terminalTitlebarBorderBottomColor: 'var(--fmq-code-rule)',
            terminalTitlebarDotsForeground: 'var(--fmq-stage-line)',
            terminalTitlebarDotsOpacity: '1',
            frameBoxShadowCssValue: 'none',
            inlineButtonBorder: 'var(--fmq-code-rule)',
          },
        },
      },
      sidebar: [
        { label: 'Start', items: [{ autogenerate: { directory: 'start' } }] },
        { label: 'Concepts', items: [{ autogenerate: { directory: 'concepts' } }] },
        { label: 'Guides', items: [{ autogenerate: { directory: 'guides' } }] },
        { label: 'Shared memory', items: [{ autogenerate: { directory: 'shmem' } }] },
        { label: 'Tools', items: [{ autogenerate: { directory: 'tools' } }] },
        { label: 'Reference', items: [{ autogenerate: { directory: 'reference' } }] },
        { label: 'Examples', collapsed: true, items: [{ autogenerate: { directory: 'examples' } }] },
        { label: 'Project', collapsed: true, items: [{ autogenerate: { directory: 'project' } }] },
        {
          label: 'Code',
          items: [
            { label: 'API reference', link: '/api/' },
            { label: 'Source browser', link: '/source/' },
          ],
        },
      ],
      plugins: [
        starlightLinksValidator({
          errorOnRelativeLinks: false,
          // Generated pages, and the row anchors of <OptionsTable> (the validator sees only
          // headings). scripts/check-links.mjs checks these links in dist/.
          exclude: ['/FairMQ/api/**', '/FairMQ/source/**', '/FairMQ/reference/options/#opt-*'],
        }),
      ],
    }),
  ],
});
