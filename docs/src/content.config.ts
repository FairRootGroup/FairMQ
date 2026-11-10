import { defineCollection } from 'astro:content';
import { z } from 'astro/zod';
import { docsLoader } from '@astrojs/starlight/loaders';
import { docsSchema } from '@astrojs/starlight/schema';

export const collections = {
  docs: defineCollection({
    loader: docsLoader(),
    schema: docsSchema({
      extend: z.object({
        /** Text above the title. The default is the name of the sidebar section. */
        eyebrow: z.string().optional(),
        /** Kind badge after the title, for API pages. The title is then set in the code font. */
        titleKind: z.string().optional(),
        /** Title for the search results, if it differs from the page title. */
        searchTitle: z.string().optional(),
        /** First FairMQ release with the feature that the page describes, e.g. "v1.11". */
        since: z.string().optional(),
        /** Source files that the page describes, relative to the repository root. */
        code: z.array(z.string()).default([]),
        /** Links in the right column under "Guides". The API pages use it. */
        guides: z.array(z.object({ label: z.string(), link: z.string() })).default([]),
      }),
    }),
  }),
};
