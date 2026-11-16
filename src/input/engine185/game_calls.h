#ifndef MYTH_185_GAME_CALLS_H
#define MYTH_185_GAME_CALLS_H
/* Isolated Build 471b prototypes; not a production bridge entry point.
 * Caller must validate the executable hash, execute on the original engine
 * thread, and validate slots/counts/pointers before invoking either function.
 * This layer only adapts the original x86 calling conventions.
 */
typedef unsigned short Myth185Word;
typedef int Myth185Coord;
void movement185(const Myth185Word options[4], int unitCount,
                 const Myth185Word *slots, int pointCount,
                 const Myth185Coord *points);
void formation185(const Myth185Word options[4], int player,
                  const Myth185Word *slots, int unitCount,
                  const Myth185Coord *points, Myth185Coord *output,
                  Myth185Word *facings, int pointCount);
#endif
