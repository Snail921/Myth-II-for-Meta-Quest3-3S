# Creating sprite materials solo: from one warrior shield to 84 collections

[日本語](MATERIAL_WORKFLOW.ja.md) · [Material Editor guide](MATERIALS.md)

One artist directed the material work and used an assistant connected to the open editor to apply decisions across poses and viewing directions. The artist decided whether a piece of armor was visible from behind, or where a tree should stay dark when viewed against the sun. The distributed pack contains 84 collections and 23,221 image records, of which 20,939 have the PBR flag. These figures count exported records. They do not measure hand-painted frames or hours saved.

## Establish the recipe on one character

For the warrior, the artist specified Metallic & Smoothness (MS) of 0.6 for armor and shield, 0.85 for the sword and 0.1 for boots. MS controls gloss and metallic appearance together. In **Material colors**, the artist named parts such as the shield paint and selected representative colors. **Direct connection** identified the current collection. Exact color matches left gaps across shaded or painted surfaces, so neighboring shades and the shape of each part mattered.

The work proceeded one part at a time. After correcting the painted shield, the artist requested armor at 0.6 while preserving the finished shield, then the sword at 0.85. Starting at image 290, the wounded poses called for a fresh mask distinguishing armor at 0.6 from blood at 0.85. A request specifying the image range, part, value and finished areas to preserve made each revision small.

## Correct an ambiguous part in the pose where it fails

Shiver's skin and breastplate have similar colors. A selection that looked right from the front assigned metal to bare skin in rear views. The correction depended on recognizing a pose in which the breastplate was hidden. On the Heron Guard, the artist requested MS 0 on facial pixels mistaken for armor. On the armed villager, the gun was correctly located but the selection had holes, so the request was to fill the connected part.

The next dwarf reused the previous recipe: beard Thickness 0.2, body Thickness 0.5 and MS 0.2, boots MS 0.1. The artist updated references for the parts whose artwork differed. On characters, Thickness was generally applied where the part touched the outer silhouette, feathering slightly inward only where there was room. On the barbarian, the histogram prompted a closer look: dark outlines had escaped the skin selection. The artist added outline colors to the references.

## Decide where sunlight should get through

Thickness 1 blocks transmission. Lower values let more light through. For the willow in 01 scenery, images 111–116, the artist specified trunks, branches and dense overlapping foliage at 1, with scattered thin spots at 0.5 along the edges of individual leaf clusters. A uniform outline would erase the differences between clusters. Low shrubs retained thickness near the ground and center. Narrow grass blades became gradually thicker from the tips toward the middle. Each plant type received its own instructions.

Sometimes the defect belonged to another layer. On the melons in images 137–144, stripes in the artwork had become ridges in Height and Normal. The requested correction was to restore the smooth elongated shape in those layers, leaving the material values alone.

## Before moving to the next collection

1. Switch between **Source** and **Metallic & Smoothness** or **Thickness**. Sample front, rear and wounded poses. Use the histogram to check the distribution, then look at the image to locate the affected pixels.
2. Give the collection name, image numbers and parts to preserve when requesting a revision. When two editors were open, specifying “01 scenery, 111–116” identified the intended work. The [API guide](SPRITE_EDITOR_API.md) covers session checks, previews and saving.
3. Save the editable **.smproj**, verify the per-image **PBR** flags in Source, and **Export for Quest**. Separate normal and PBR output folders let the collection browser mark existing exports. A mark confirms a matching filename; check whether edits made after that export still need exporting.
4. Transfer the PBR folder with Plugin Manager and enable the corresponding category in the launcher. In Willow Creek, the artist checked the sky reflected in the warrior's helmet and looked up through a backlit canopy. Use the specular-only or SSS-only diagnostic view to isolate a subtle contribution, then judge the result in normal rendering. Record the LOD start and smoothing distances when evaluating the transition.
