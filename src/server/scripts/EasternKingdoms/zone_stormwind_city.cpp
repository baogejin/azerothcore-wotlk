/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Stormwind_City
SD%Complete: 100
SDComment: Quest support: 1640, 1447, 4185, 11223, 434.
SDCategory: Stormwind City
EndScriptData */

/* ContentData
npc_archmage_malin
npc_bartleby
npc_tyrion
npc_tyrion_spybot
npc_marzon_silent_blade
npc_lord_gregor_lescovar
EndContentData */

#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"

/*######
## npc_bartleby
######*/

enum Bartleby
{
    QUEST_BEAT          = 1640
};

class npc_bartleby : public CreatureScript
{
public:
    npc_bartleby() : CreatureScript("npc_bartleby") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_BEAT)
        {
            creature->SetFaction(FACTION_ENEMY);
            creature->AI()->AttackStart(player);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bartlebyAI(creature);
    }

    struct npc_bartlebyAI : public ScriptedAI
    {
        npc_bartlebyAI(Creature* creature) : ScriptedAI(creature)
        {
            m_uiNormalFaction = creature->GetFaction();
        }

        uint32 m_uiNormalFaction;

        void Reset() override
        {
            if (me->GetFaction() != m_uiNormalFaction)
                me->SetFaction(m_uiNormalFaction);
        }

        void AttackedBy(Unit* pAttacker) override
        {
            if (me->GetVictim())
                return;

            if (me->IsFriendlyTo(pAttacker))
                return;

            AttackStart(pAttacker);
        }

        void DamageTaken(Unit* pDoneBy, uint32& uiDamage, DamageEffectType, SpellSchoolMask) override
        {
            if (pDoneBy && (uiDamage >= me->GetHealth() || me->HealthBelowPctDamaged(15, uiDamage)))
            {
                //Take 0 damage
                uiDamage = 0;

                if (Player* player = pDoneBy->ToPlayer())
                    player->AreaExploredOrEventHappens(QUEST_BEAT);
                EnterEvadeMode();
            }
        }
    };
};

/*######
## npc_lord_gregor_lescovar
######*/

enum LordGregorLescovar
{
    SAY_GUARD_2    = 0,
    SAY_LESCOVAR_2 = 0,
    SAY_LESCOVAR_3 = 1,
    SAY_LESCOVAR_4 = 2,
    SAY_MARZON_1   = 0,
    SAY_MARZON_2   = 1,
    SAY_TYRION_2   = 1,

    NPC_STORMWIND_ROYAL = 1756,
    NPC_MARZON_BLADE    = 1755,
    NPC_TYRION          = 7766,

    QUEST_THE_ATTACK    = 434
};

class npc_lord_gregor_lescovar : public CreatureScript
{
public:
    npc_lord_gregor_lescovar() : CreatureScript("npc_lord_gregor_lescovar") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lord_gregor_lescovarAI(creature);
    }

    struct npc_lord_gregor_lescovarAI : public npc_escortAI
    {
        npc_lord_gregor_lescovarAI(Creature* creature) : npc_escortAI(creature) { }

        uint32 uiTimer;
        uint32 uiPhase;

        ObjectGuid MarzonGUID;

        void Reset() override
        {
            uiTimer = 0;
            uiPhase = 0;

            MarzonGUID.Clear();
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->DisappearAndDie();

            if (Creature* pMarzon = ObjectAccessor::GetCreature(*me, MarzonGUID))
            {
                if (pMarzon->IsAlive())
                    pMarzon->DisappearAndDie();
            }
        }

        void JustEngagedWith(Unit* who) override
        {
            if (Creature* pMarzon = ObjectAccessor::GetCreature(*me, MarzonGUID))
            {
                if (pMarzon->IsAlive() && !pMarzon->IsInCombat())
                    pMarzon->AI()->AttackStart(who);
            }
        }

        void WaypointReached(uint32 waypointId) override
        {
            switch (waypointId)
            {
                case 14:
                    SetEscortPaused(true);
                    Talk(SAY_LESCOVAR_2);
                    uiTimer = 3000;
                    uiPhase = 1;
                    break;
                case 16:
                    SetEscortPaused(true);
                    if (Creature* pMarzon = me->SummonCreature(NPC_MARZON_BLADE, -8411.360352f, 480.069733f, 123.760895f, 4.941504f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000))
                    {
                        pMarzon->GetMotionMaster()->MovePoint(0, -8408.000977f, 468.611450f, 123.759903f);
                        MarzonGUID = pMarzon->GetGUID();
                    }
                    uiTimer = 2000;
                    uiPhase = 4;
                    break;
            }
        }
        //TO-DO: We don't have movemaps, also we can't make 2 npcs walks to one point propperly (and we can not use escort ai, because they are 2 different spawns and with same entry), because of it we make them, disappear.
        void DoGuardsDisappearAndDie()
        {
            std::list<Creature*> GuardList;
            me->GetCreatureListWithEntryInGrid(GuardList, NPC_STORMWIND_ROYAL, 8.0f);
            if (!GuardList.empty())
            {
                for (std::list<Creature*>::const_iterator itr = GuardList.begin(); itr != GuardList.end(); ++itr)
                {
                    if (Creature* pGuard = *itr)
                        pGuard->DisappearAndDie();
                }
            }
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (uiPhase)
            {
                if (uiTimer <= uiDiff)
                {
                    switch (uiPhase)
                    {
                        case 1:
                            if (Creature* pGuard = me->FindNearestCreature(NPC_STORMWIND_ROYAL, 8.0f, true))
                                pGuard->AI()->Talk(SAY_GUARD_2);
                            uiTimer = 3000;
                            uiPhase = 2;
                            break;
                        case 2:
                            DoGuardsDisappearAndDie();
                            uiTimer = 2000;
                            uiPhase = 3;
                            break;
                        case 3:
                            SetEscortPaused(false);
                            uiTimer = 0;
                            uiPhase = 0;
                            break;
                        case 4:
                            Talk(SAY_LESCOVAR_3);
                            uiTimer = 0;
                            uiPhase = 0;
                            break;
                        case 5:
                            if (Creature* pMarzon = ObjectAccessor::GetCreature(*me, MarzonGUID))
                                pMarzon->AI()->Talk(SAY_MARZON_1);
                            uiTimer = 3000;
                            uiPhase = 6;
                            break;
                        case 6:
                            Talk(SAY_LESCOVAR_4);
                            if (Player* player = GetPlayerForEscort())
                                player->GroupEventHappens(QUEST_THE_ATTACK, me);
                            uiTimer = 2000;
                            uiPhase = 7;
                            break;
                        case 7:
                            if (Creature* pTyrion = me->FindNearestCreature(NPC_TYRION, 20.0f, true))
                                pTyrion->AI()->Talk(SAY_TYRION_2);
                            if (Creature* pMarzon = ObjectAccessor::GetCreature(*me, MarzonGUID))
                                pMarzon->SetFaction(FACTION_MONSTER);
                            me->SetFaction(FACTION_MONSTER);
                            uiTimer = 0;
                            uiPhase = 0;
                            break;
                    }
                }
                else uiTimer -= uiDiff;
            }
            npc_escortAI::UpdateAI(uiDiff);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

/*######
## npc_marzon_silent_blade
######*/

class npc_marzon_silent_blade : public CreatureScript
{
public:
    npc_marzon_silent_blade() : CreatureScript("npc_marzon_silent_blade") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_marzon_silent_bladeAI(creature);
    }

    struct npc_marzon_silent_bladeAI : public ScriptedAI
    {
        npc_marzon_silent_bladeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetWalk(true);
        }

        void Reset() override
        {
            me->RestoreFaction();
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(SAY_MARZON_2);

            if (me->IsSummon())
            {
                if (Unit* summoner = me->ToTempSummon()->GetSummonerUnit())
                {
                    if (summoner->GetTypeId() == TYPEID_UNIT && summoner->IsAlive() && !summoner->IsInCombat())
                        summoner->ToCreature()->AI()->AttackStart(who);
                }
            }
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->DisappearAndDie();

            if (me->IsSummon())
            {
                if (Unit* summoner = me->ToTempSummon()->GetSummonerUnit())
                {
                    if (summoner->GetTypeId() == TYPEID_UNIT && summoner->IsAlive())
                        summoner->ToCreature()->DisappearAndDie();
                }
            }
        }

        void MovementInform(uint32 uiType, uint32 /*uiId*/) override
        {
            if (uiType != POINT_MOTION_TYPE)
                return;

            if (me->IsSummon())
            {
                Unit* summoner = me->ToTempSummon()->GetSummonerUnit();
                if (summoner && summoner->GetTypeId() == TYPEID_UNIT && summoner->IsAIEnabled)
                {
                    npc_lord_gregor_lescovar::npc_lord_gregor_lescovarAI* ai =
                        CAST_AI(npc_lord_gregor_lescovar::npc_lord_gregor_lescovarAI, summoner->GetAI());
                    if (ai)
                    {
                        ai->uiTimer = 2000;
                        ai->uiPhase = 5;
                    }
                    //me->ChangeOrient(0.0f, summoner);
                }
            }
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

/*######
## npc_tyrion_spybot
######*/

enum TyrionSpybot
{
    SAY_QUEST_ACCEPT_ATTACK  = 0,
    SAY_SPYBOT_1             = 1,
    SAY_SPYBOT_2             = 2,
    SAY_SPYBOT_3             = 3,
    SAY_SPYBOT_4             = 4,
    SAY_TYRION_1             = 0,
    SAY_GUARD_1              = 1,
    SAY_LESCOVAR_1           = 3,

    NPC_PRIESTESS_TYRIONA    = 7779,
    NPC_LORD_GREGOR_LESCOVAR = 1754,
};

class npc_tyrion_spybot : public CreatureScript
{
public:
    npc_tyrion_spybot() : CreatureScript("npc_tyrion_spybot") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tyrion_spybotAI(creature);
    }

    struct npc_tyrion_spybotAI : public npc_escortAI
    {
        npc_tyrion_spybotAI(Creature* creature) : npc_escortAI(creature) { }

        uint32 uiTimer;
        uint32 uiPhase;

        void Reset() override
        {
            uiTimer = 0;
            uiPhase = 0;
        }

        void WaypointReached(uint32 waypointId) override
        {
            switch (waypointId)
            {
                case 1:
                    SetEscortPaused(true);
                    uiTimer = 2000;
                    uiPhase = 1;
                    break;
                case 5:
                    SetEscortPaused(true);
                    Talk(SAY_SPYBOT_1);
                    uiTimer = 2000;
                    uiPhase = 5;
                    break;
                case 17:
                    SetEscortPaused(true);
                    Talk(SAY_SPYBOT_3);
                    uiTimer = 3000;
                    uiPhase = 8;
                    break;
            }
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (uiPhase)
            {
                if (uiTimer <= uiDiff)
                {
                    switch (uiPhase)
                    {
                        case 1:
                            Talk(SAY_QUEST_ACCEPT_ATTACK);
                            uiTimer = 3000;
                            uiPhase = 2;
                            break;
                        case 2:
                            if (Creature* pTyrion = me->FindNearestCreature(NPC_TYRION, 10.0f))
                            {
                                if (Player* player = GetPlayerForEscort())
                                    pTyrion->AI()->Talk(SAY_TYRION_1, player);
                            }
                            uiTimer = 3000;
                            uiPhase = 3;
                            break;
                        case 3:
                            me->UpdateEntry(NPC_PRIESTESS_TYRIONA);
                            uiTimer = 2000;
                            uiPhase = 4;
                            break;
                        case 4:
                            SetEscortPaused(false);
                            uiPhase = 0;
                            uiTimer = 0;
                            break;
                        case 5:
                            if (Creature* pGuard = me->FindNearestCreature(NPC_STORMWIND_ROYAL, 10.0f, true))
                                pGuard->AI()->Talk(SAY_GUARD_1);
                            uiTimer = 3000;
                            uiPhase = 6;
                            break;
                        case 6:
                            Talk(SAY_SPYBOT_2);
                            uiTimer = 3000;
                            uiPhase = 7;
                            break;
                        case 7:
                            SetEscortPaused(false);
                            uiTimer = 0;
                            uiPhase = 0;
                            break;
                        case 8:
                            if (Creature* pLescovar = me->FindNearestCreature(NPC_LORD_GREGOR_LESCOVAR, 10.0f))
                                pLescovar->AI()->Talk(SAY_LESCOVAR_1);
                            uiTimer = 3000;
                            uiPhase = 9;
                            break;
                        case 9:
                            Talk(SAY_SPYBOT_4);
                            uiTimer = 3000;
                            uiPhase = 10;
                            break;
                        case 10:
                            if (Creature* pLescovar = me->FindNearestCreature(NPC_LORD_GREGOR_LESCOVAR, 10.0f))
                            {
                                if (Player* player = GetPlayerForEscort())
                                {
                                    CAST_AI(npc_lord_gregor_lescovar::npc_lord_gregor_lescovarAI, pLescovar->AI())->Start(false, false, player->GetGUID());
                                    CAST_AI(npc_lord_gregor_lescovar::npc_lord_gregor_lescovarAI, pLescovar->AI())->SetMaxPlayerDistance(200.0f);
                                }
                            }
                            me->DisappearAndDie();
                            uiTimer = 0;
                            uiPhase = 0;
                            break;
                    }
                }
                else uiTimer -= uiDiff;
            }
            npc_escortAI::UpdateAI(uiDiff);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

/*######
## npc_tyrion
######*/

enum Tyrion
{
    NPC_TYRION_SPYBOT = 8856
};

class npc_tyrion : public CreatureScript
{
public:
    npc_tyrion() : CreatureScript("npc_tyrion") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_THE_ATTACK)
        {
            if (Creature* pSpybot = creature->FindNearestCreature(NPC_TYRION_SPYBOT, 5.0f, true))
            {
                CAST_AI(npc_tyrion_spybot::npc_tyrion_spybotAI, pSpybot->AI())->Start(false, false, player->GetGUID());
                CAST_AI(npc_tyrion_spybot::npc_tyrion_spybotAI, pSpybot->AI())->SetMaxPlayerDistance(200.0f);
            }
            return true;
        }
        return false;
    }
};

enum
{
    QUEST_STORMWIND_RENDEZVOUS = 6402,
    QUEST_THE_GREAT_MASQUERADE = 6403,

    NPC_REGINALD_WINDSOR = 12580,
    NPC_ONYXIA_ELITE_GUARD = 12739,
    NPC_ANDUIN_WRYNN = 1747,
    NPC_BOLVAR_FORDRAGON = 1748,
    NPC_KATRANA_PRESTOR = 1749,
    NPC_STORMWIND_CITY_GUARD = 68,
    NPC_STORMWIND_CITY_PATROL = 1976,
    NPC_MERCUTIO = 12581,
    NPC_MARCUS_JONATHAN = 466,
    NPC_STORMWIND_ROYAL_GUARD = 1756,
    NPC_LADY_ONYXIA = 12756,
    NPC_ROWE = 17804,

    FACTION_BOLVAR_COMBAT = 11,
    FACTION_BOLVAR_NORMAL = 12,

    GOSSIP_ROWE_COMPLETED = 9066,
    GOSSIP_ROWE_READY = 9065,
    GOSSIP_ROWE_BUSY = 9064,
    GOSSIP_ROWE_NOTHING = 9063,

    GO_FLARE_OF_JUSTICE = 181987,

    SAY_SIGNAL_SENT = 14389,
    SAY_HISS = 8245,
    SAY_WINDSOR1 = 8091,
    SAY_WINDSOR2 = 8090,
    SAY_WINDSOR3 = 8107,
    SAY_WINDSOR4 = 8109,
    SAY_ONYXIA1 = 8119,
    SAY_MARCUS1 = 8121,
    SAY_WINDSOR5 = 8123,
    SAY_WINDSOR6 = 8133,
    SAY_MARCUS2 = 8124,
    SAY_MARCUS3 = 8125,
    SAY_MARCUS4 = 8132,
    SAY_WINDSOR7 = 8126,
    SAY_WINDSOR8 = 8134,
    SAY_MARCUS5 = 8127,
    SAY_MARCUS6 = 8128,
    SAY_MARCUS7 = 8129,
    SAY_MARCUS8 = 8130,
    SAY_WINDSOR9 = 8205,
    SAY_WINDSOR10 = 8206,
    SAY_WINDSOR11 = 8207,
    SAY_WINDSOR12 = 8208,
    SAY_WINDSOR13 = 8210,
    SAY_BOLVAR1 = 8212,
    SAY_WINDSOR14 = 8211,
    SAY_ONYXIA2 = 8214,
    SAY_ONYXIA3 = 8215,
    SAY_ONYXIA4 = 8216,
    SAY_WINDSOR15 = 8218,
    SAY_WINDSOR16 = 8226,
    SAY_WINDSOR17 = 8227,
    SAY_WINDSOR18 = 8219,
    SAY_WINDSOR19 = 8228,
    SAY_BOLVAR2 = 8236,
    SAY_ONYXIA5 = 8235,
    SAY_BOLVAR3 = 8237,
    SAY_ONYXIA6 = 8238,
    SAY_ONYXIA7 = 8239,
    SAY_WINDSOR20 = 8247,
    SAY_ONYXIA8 = 8246,
    SAY_ONYXIA9 = 8248,
    SAY_BOVLAR4 = 8266,
    SAY_BOLVAR5 = 8249,
    SAY_WINDSOR21 = 8250,
    SAY_WINDSOR22 = 8251,

    MOUNT_WINDSOR = 2410,

    SPELL_GREATER_INVISIBILITY = 16380,
    SPELL_INVISIBILITY = 23452,
    SPELL_WINDSOR_DEATH = 20465,
    SPELL_WINSOR_READ_TABLETS = 20358,
    SPELL_PRESTOR_DESPAWNS = 20466,
    SPELL_WINDSOR_DISMISS_HORSE = 20000,

    SPELL_PERMANENT_FEIGN_DEATH = 29266,
};

static Position RoweWaypoints[] =
{
    { -9058.07f, 441.32f, 93.06f, 3.84f },
    { -9084.88f, 419.23f, 92.42f, 3.83f }
};

const Position WindsorSummon = { -9148.40f, 371.32f, 91.0f, 0.70f };

static Position WindsorWaypoints[] =
{
    { -9050.406250f, 443.974792f, 93.056458f, 0.659825f },
    { -8968.008789f, 509.771759f, 96.350754f, 0.679460f },
    { -8954.638672f, 519.920410f, 96.355453f, 0.680187f },
    { -8933.738281f, 500.683533f, 93.842247f, 5.941573f },
    { -8923.248047f, 496.464294f, 93.858475f, 0.688045f },
    { -8907.830078f, 509.035645f, 93.842529f, 2.163023f },
    { -8927.302734f, 542.173523f, 94.291695f, 0.606364f },
    { -8825.773438f, 623.565918f, 93.838066f, 5.484471f },
    { -8795.209961f, 590.400269f, 97.495560f, 0.644063f },
    { -8769.717773f, 608.193298f, 97.130692f, 5.507248f },
    { -8736.326172f, 574.955811f, 97.385048f, 3.886185f },
    { -8749.043945f, 560.525330f, 97.400307f, 5.502535f },
    { -8730.701172f, 540.466370f, 101.105370f, 5.431850f },
    { -8713.182617f, 519.765442f, 97.185402f, 0.707678f },
    { -8673.321289f, 554.135986f, 97.267708f, 6.174048f },
    { -8651.262695f, 551.696045f, 97.002983f, 5.525308f },
    { -8618.138672f, 518.573425f, 103.123642f, 5.497819f },
    { -8566.013672f, 465.536804f, 104.597160f, 5.481625f },
    { -8548.403320f, 466.680695f, 104.533554f, 5.387382f },
    { -8529.294922f, 443.376495f, 104.917046f, 5.399162f },
    { -8507.087891f, 415.847321f, 108.385857f, 5.371674f },
    { -8486.496094f, 389.750427f, 108.590248f, 5.391308f },
    { -8455.687500f, 351.054321f, 120.885910f, 5.391308f },
    { -8446.392578f, 339.602783f, 121.329506f, 5.359892f }
};

static Position WindsorEventMove[] =
{
    { -8964.973633f, 512.194519f, 96.355247f, 3.835189f },
    { -8963.196289f, 510.056549f, 96.355240f, 3.835189f },
    { -8961.235352f, 507.696808f, 96.595337f, 3.835189f },
    { -8959.596680f, 505.725403f, 96.595490f, 3.835189f },
    { -8967.410156f, 515.123535f, 96.354881f, 3.835189f },
    { -8968.840820f, 516.844482f, 96.595253f, 3.835189f },
    { -8970.687500f, 519.065796f, 96.595245f, 3.835189f },
    { -8958.585938f, 506.907959f, 96.595634f, 2.294317f },
    { -8960.827148f, 505.079407f, 96.593971f, 2.255047f },
    { -8962.866211f, 503.415009f, 96.591331f, 2.255047f },
    { -8969.562500f, 520.014587f, 96.595673f, 5.388790f },
    { -8971.773438f, 518.239868f, 96.594200f, 5.388790f },
    { -8973.923828f, 516.513611f, 96.590904f, 5.475183f },
    { -8976.549805f, 514.405701f, 96.590057f, 5.388790f },
    { -8506.340820f, 338.364441f, 120.88584f, 6.219385f },
    { -8449.006836f, 337.693451f, 121.32955f, 5.826686f }
};

uint32 GetRandomGuardText()
{
    switch (urand(0, 7))
    {
    case 0:
        return 8167; // Light be with you, sir.
    case 1:
        return 8170; // You are an inspiration to us all, sir.
    case 2:
        return 8172; // There walks a hero.
    case 3:
        return 8175; // Make way!
    case 4:
        return 8177; // We are but dirt beneath your feet, sir.
    case 5:
        return 8180; // A moment I shall remember for always.
    case 6:
        return 8183; // ...nerves of thorium.
    }
    return 8184; // A living legend... 
}

struct npc_squire_roweAI;

struct npc_reginald_windsorAI : ScriptedAI
{
    npc_reginald_windsorAI(Creature* pCreature) : ScriptedAI(pCreature) {}
    uint32 Timer;
    uint32 Tick;
    ObjectGuid GuardsGUIDs[30];
    ObjectGuid DragsGUIDs[10];
    ObjectGuid playerGUID;
    bool Begin;
    bool SummonHorse;
    bool ShooHorse;
    bool BeginQuest;
    bool NeedCheck;
    bool GuardNeed[6];
    bool PhaseFinale;
    bool TheEnd;
    bool CombatJustEnded;
    bool GreetPlayer;
    bool QuestAccepted;
    bool m_bRoweKnows;
    uint32 FinalTimer;
    uint32 GuardTimer[6];
    uint32 m_uiDespawnTimer;
    ObjectGuid m_squireRoweGuid;

    Player* GetPlayer() const
    {
        return ObjectAccessor::FindPlayer(playerGUID);
    }
    Creature* GetGuard(uint8 num) const
    {
        return me->GetMap()->GetCreature(GuardsGUIDs[num]);
    }

    void Reset() override
    {
        m_bRoweKnows = false;
        m_squireRoweGuid.Clear();

        m_uiDespawnTimer = 5 * MINUTE * IN_MILLISECONDS;
        Timer = 3000;
        Tick = 0;
        GreetPlayer = false;
        QuestAccepted = false;
        BeginQuest = false;
        Begin = true;
        NeedCheck = false;
        PhaseFinale = false;
        TheEnd = false;
        CombatJustEnded = false;
        ShooHorse = false;
        SummonHorse = false;

        for (uint8 i = 0; i < 6; i++)
        {
            GuardTimer[i] = 0;
            GuardNeed[i] = false;
        }

        //me->EnableMoveInLosEvent(); //不清楚干啥的
    }
    //void ResetCreature() override;
    void JustDied(Unit* pKiller) override
    {
        //好像没有调用到哦
        PokeRowe();
        me->DespawnOrUnsummon(5 * MINUTE * IN_MILLISECONDS);
    }
    void PokeRowe();
    void CompleteQuest()
    {
        if (Player* pPlayer = GetPlayer())
        {
            if (Group* jGroup = pPlayer->GetGroup())
            {
                for (GroupReference* pRef = jGroup->GetFirstMember(); pRef != nullptr; pRef = pRef->next())
                {
                    if (pRef->GetSource()->GetQuestStatus(QUEST_THE_GREAT_MASQUERADE) == QUEST_STATUS_INCOMPLETE)
                        pRef->GetSource()->CompleteQuest(QUEST_THE_GREAT_MASQUERADE);
                }
            }
            else
            {
                if (pPlayer->GetQuestStatus(QUEST_THE_GREAT_MASQUERADE) == QUEST_STATUS_INCOMPLETE)
                    pPlayer->CompleteQuest(QUEST_THE_GREAT_MASQUERADE);
            }
        }
    }
    void EndScene()
    {
        if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
        {
            float X = 0.0f;
            float Y = 0.0f;
            float Z = 0.0f;
            float orientation = 0.0f;
            Bolvar->SetStandState(UNIT_STAND_STATE_STAND);
            Bolvar->GetRespawnPosition(X, Y, Z, &orientation);
            Bolvar->GetMotionMaster()->MovePoint(0, { X, Y, Z,orientation }, false);
        }

        if (Creature* Anduin = me->FindNearestCreature(NPC_ANDUIN_WRYNN, 150.0f))
        {
            float X = 0.0f;
            float Y = 0.0f;
            float Z = 0.0f;
            float orientation = 0.0f;
            Anduin->GetRespawnPosition(X, Y, Z, &orientation);
            Anduin->GetMotionMaster()->MovePoint(0, { X, Y, Z, orientation }, false);
            float x = Anduin->GetPositionX() - X;
            float y = Anduin->GetPositionY() - Y;
            FinalTimer = 1000 + sqrt((x * x) + (y * y)) / (Anduin->GetSpeed(MOVE_WALK) * 0.001f);
        }
        TheEnd = true;

        if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
        {
            Onyxia->RemoveAurasDueToSpell(SPELL_GREATER_INVISIBILITY);
            Onyxia->CastSpell(Onyxia, SPELL_INVISIBILITY, true);
        }
    }

    void MoveInLineOfSight(Unit* Victim) override
    {
        if (Victim && Victim->IsAlive())
        {
            if (Victim->GetEntry() == NPC_STORMWIND_CITY_GUARD ||
                Victim->GetEntry() == NPC_STORMWIND_ROYAL_GUARD ||
                Victim->GetEntry() == NPC_STORMWIND_CITY_PATROL)
            {
                if (Victim->GetDistance2d(me) < 8.0f && NeedCheck)
                {
                    bool Continuer = true;
                    for (ObjectGuid guid : GuardsGUIDs)
                    {
                        if (Victim->GetGUID() == guid || me->GetPositionY() < 360)
                            Continuer = false;
                    }
                    if (Continuer)
                    {
                        Victim->SetFacingToObject(me);
                        Victim->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                        Victim->Say(GetRandomGuardText());
                        int Var = 0;
                        while (GuardsGUIDs[Var] && Var < 29)
                            Var++;

                        GuardsGUIDs[Var] = Victim->GetGUID();
                    }
                }
            }
        }
    }
    //void SpellHit(SpellCaster* /*pCaster*/, SpellEntry const* pSpellEntry) override;
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        //没用，没有调用到，很奇怪
        if (spell->Id == SPELL_WINDSOR_DEATH)
        {
            //me->SetFeignDeath(true);
            //me->setDeathState(JUST_DIED);
            //me->AddUnitState(UNIT_STATE_DIED);
            //me->setDeathState(CORPSE, false);
            //DoCastSelf(SPELL_PERMANENT_FEIGN_DEATH, true);
        }
    }
    void UpdateAI(uint32 const uiDiff) override
    {
        if (TheEnd)
        {
            if (FinalTimer < uiDiff)
            {
                float X = 0.0f;
                float Y = 0.0f;
                float Z = 0.0f;
                float O = 0.0f;
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                {
                    Bolvar->GetRespawnPosition(X, Y, Z, &O);
                    Bolvar->SetFacingTo(O);
                }
                if (Creature* Anduin = me->FindNearestCreature(NPC_ANDUIN_WRYNN, 150.0f))
                {
                    Anduin->GetRespawnPosition(X, Y, Z, &O);
                    Anduin->SetFacingTo(O);
                }
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                {
                    Onyxia->GetRespawnPosition(X, Y, Z, &O);
                    Onyxia->SetFacingTo(O);
                    Onyxia->SetEntry(NPC_KATRANA_PRESTOR);
                    Onyxia->UpdateEntry(NPC_KATRANA_PRESTOR);
                }
                PokeRowe();
                TheEnd = false;
                me->setDeathState(JUST_DIED);
            }
            else
                FinalTimer -= uiDiff;
        }
        // in case of idle / afk players
        if (m_uiDespawnTimer < uiDiff)
        {
            PokeRowe();
            me->DespawnOrUnsummon();
        }
        else
            m_uiDespawnTimer -= uiDiff;

        for (int i = 0; i < 6; i++)
        {
            if (GuardNeed[i])
            {
                if (GuardTimer[i] < uiDiff)
                {
                    if (Creature* pGuard = GetGuard(i))
                    {
                        int Var = i + 7;
                        pGuard->SetFacingTo(WindsorEventMove[Var].m_orientation);
                        pGuard->SetStandState(UNIT_STAND_STATE_KNEEL);
                        GuardNeed[i] = false;
                    }
                }
                else
                    GuardTimer[i] -= uiDiff;
            }
        }
        if (Begin)
        {
            if (me->GetDistance2d(WindsorWaypoints[0].GetPositionX(), WindsorWaypoints[0].GetPositionY()) < 2.0f)
            {
                if (Timer <= uiDiff)
                {
                    Begin = false;
                    me->SetWalk(true);
                    me->SetSpeedRate(MOVE_WALK, 1.0f);

                    if (Creature* pRowe = me->FindNearestCreature(NPC_ROWE, 50.0f))
                        pRowe->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);

                    Timer = 2000;
                    SummonHorse = true;
                }
                else
                    Timer -= uiDiff;
            }
        }

        if (SummonHorse)
        {
            if (Timer <= uiDiff)
            {
                SummonHorse = false;
                me->Dismount();
                //DoCast(me, SPELL_WINDSOR_DISMISS_HORSE, true);
                if (Creature* pMercutio = me->FindNearestCreature(NPC_MERCUTIO, 10.0f))
                {
                    pMercutio->SetWalk(false);
                    me->SetFacingToObject(pMercutio);
                }
                ShooHorse = true;
                Timer = 2000;
            }
            else
                Timer -= uiDiff;
        }

        if (ShooHorse)
        {
            if (Timer <= uiDiff)
            {
                ShooHorse = false;
                if (Creature* pMercutio = me->FindNearestCreature(NPC_MERCUTIO, 10.0f))
                {
                    me->Say(SAY_WINDSOR1);
                    pMercutio->GetMotionMaster()->MovePoint(0, -9148.395508f, 371.322174f, 90.543655f);
                }
                GreetPlayer = true;
                Timer = 2000;
            }
            else
                Timer -= uiDiff;
        }

        if (!BeginQuest)
        {
            if (GreetPlayer)
            {
                if (Timer <= uiDiff)
                {
                    Player* pPlayer = GetPlayer();
                    if (pPlayer)
                    {
                        me->SetFacingToObject(pPlayer);
                        me->Say(SAY_WINDSOR2, pPlayer);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    }
                    GreetPlayer = false;
                }
                else
                    Timer -= uiDiff;
            }
            return;
        }

        if (Timer < uiDiff)
        {
            std::list<Creature*> DragListe;
            float X = 0.0f;
            float Y = 0.0f;
            uint32 eventGardId = 6;
            switch (Tick)
            {
            case 0:
                m_uiDespawnTimer = 20 * MINUTE * IN_MILLISECONDS;
                me->Say(SAY_WINDSOR3);
                Timer = 5000;
                break;
            case 1:
                me->SetFacingTo(0.659f);
                me->Say(SAY_WINDSOR4);
                Timer = 5000;
                break;
            case 2:
                for (int i = 0; i < 6; i++)
                {
                    int Var = i + 1;
                    Creature* pSummon = me->SummonCreature(NPC_STORMWIND_CITY_GUARD,WindsorEventMove[Var], TEMPSUMMON_TIMED_DESPAWN, 240 * IN_MILLISECONDS);
                    if (pSummon)
                    {
                        GuardsGUIDs[i] = pSummon->GetGUID();
                        pSummon->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    }
                }
                if (Creature* Onyxia = me->SummonCreature(NPC_KATRANA_PRESTOR, { -9075.6f, 466.11f, 120.383f, 6.27f }, TEMPSUMMON_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
                {
                    Onyxia->SetDisplayId(11686); // invisible
                    Onyxia->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    Onyxia->Yell(SAY_ONYXIA1);
                }
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    General->GetMotionMaster()->MovePoint(0, WindsorEventMove[0]);
                    General->Dismount();
                }
                me->GetMotionMaster()->MovePoint(0, WindsorWaypoints[1]);
                X = me->GetPositionX() - WindsorWaypoints[1].GetPositionX();
                Y = me->GetPositionY() - WindsorWaypoints[1].GetPositionY();
                Timer = 1000 + sqrt(X * X + Y * Y) / (me->GetSpeed(MOVE_WALK) * 0.001f);
                break;
            case 3:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                    General->Say(SAY_MARCUS1);
                Timer = 5000;
                break;
            case 4:
                me->Say(SAY_WINDSOR5);
                Timer = 5000;
                break;
            case 5:
                me->Say(SAY_WINDSOR6);
                Timer = 5000;
                break;
            case 6:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                    General->TextEmote(SAY_MARCUS2, General);
                Timer = 5000;
                break;
            case 7:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                    General->Say(SAY_MARCUS3);
                Timer = 5000;
                break;
            case 8:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                    General->Say(SAY_MARCUS4);
                Timer = 10000;
                break;
            case 9:
                me->Say(SAY_WINDSOR7);
                Timer = 5000;
                break;
            case 10:
                me->Say(SAY_WINDSOR8);
                Timer = 5000;
                break;
            case 11:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    if (Creature* pGuard = GetGuard(0))
                        General->SetFacingToObject(pGuard);
                    General->Say(SAY_MARCUS5);
                }
                break;
            case 12:
                eventGardId = 0;
                break;
            case 13:
                eventGardId = 1;
                break;
            case 14:
                eventGardId = 2;
                Timer = 5000;
                break;
            case 15:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    if (Creature* pGuard = GetGuard(3))
                        General->SetFacingToObject(pGuard);
                    General->Say(SAY_MARCUS6);
                }
                break;
            case 16:
                eventGardId = 3;
                break;
            case 17:
                eventGardId = 4;
                break;
            case 18:
                eventGardId = 5;
                Timer = 5000;
                break;
            case 19:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    General->SetFacingToObject(me);
                    General->Say(SAY_MARCUS7);
                }
                Timer = 5000;
                break;
            case 20:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                    General->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                Timer = 5000;
                break;
            case 21:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                    General->Say(SAY_MARCUS8);
                Timer = 5000;
                break;
            case 22:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    General->GetMotionMaster()->MovePoint(0, WindsorEventMove[13]);
                    X = General->GetPositionX() - WindsorEventMove[13].GetPositionX();
                    Y = General->GetPositionY() - WindsorEventMove[13].GetPositionY();
                    Timer = 1000 + sqrt(X * X + Y * Y) / (me->GetSpeed(MOVE_WALK) * 0.001f);
                }
                else Timer = 1000;
                break;
            case 23:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    General->SetStandState(UNIT_STAND_STATE_KNEEL);
                    General->SetFacingTo(WindsorEventMove[13].GetOrientation());
                    me->SetFacingToObject(General);
                    me->Say(SAY_WINDSOR9);
                }
                Timer = 10000;
                break;
            case 24:
                if (Creature* pGuard = GetGuard(0))
                    me->SetFacingToObject(pGuard);
                me->Say(SAY_WINDSOR10);
                Timer = 5000;
                break;
            case 28:
                NeedCheck = true;
                break;
            case 29:
                if (Creature* General = me->FindNearestCreature(NPC_MARCUS_JONATHAN, 150.0f))
                {
                    General->Respawn(true);
                }
                break;
            case 43:
                m_uiDespawnTimer = 10 * MINUTE * IN_MILLISECONDS;
                BeginQuest = false;
                me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->Say(SAY_WINDSOR11);
                break;
            case 50:
                me->Say(SAY_WINDSOR13);
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    Bolvar->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                    Onyxia->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
                Timer = 4000;
                break;
            case 51:
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    Bolvar->Say(SAY_BOLVAR1);
                break;
            case 52:
                if (Creature* Anduin = me->FindNearestCreature(NPC_ANDUIN_WRYNN, 150.0f))
                {
                    Anduin->SetWalk(false);
                    Anduin->GetMotionMaster()->MovePoint(0, WindsorEventMove[14]);
                }
                Timer = 5000;
                break;
            case 53:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                {
                    me->SetFacingToObject(Onyxia);
                    me->Say(SAY_WINDSOR14);
                }
                Timer = 5000;
                break;
            case 54:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                {
                    Onyxia->TextEmote(SAY_ONYXIA2, Onyxia);
                    Onyxia->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
                }
                Timer = 4000;
                break;
            case 55:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                    Onyxia->Say(SAY_ONYXIA3);
                Timer = 10000;
                break;
            case 56:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                    Onyxia->Say(SAY_ONYXIA4);
                Timer = 5000;
                break;
            case 57:
                me->Say(SAY_WINDSOR15);
                Timer = 5000;
                break;
            case 58:
                me->TextEmote(SAY_WINDSOR16, me);
                Timer = 5000;
                break;
            case 59:
                me->Say(SAY_WINDSOR17);
                Timer = 4000;
                break;
            case 60:
                me->Say(SAY_WINDSOR18);
                Timer = 5000;
                break;
            case 61:
                me->TextEmote(SAY_WINDSOR19, me);
                Timer = 1000;
                break;
            case 62:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                    // me->CastSpell(Onyxia, SPELL_WINSOR_READ_TABLETS, false);
                   me->HandleEmoteCommand(EMOTE_STATE_POINT);
                Timer = 5000;
                break;
            case 63:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_KATRANA_PRESTOR, 150.0f))
                {
                    Onyxia->UpdateEntry(NPC_LADY_ONYXIA);
                    //Onyxia->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING | UNIT_FLAG_IMMUNE_TO_NPC);
                    Onyxia->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_NPC);
                }
                Timer = 1000;
                break;
            case 64:
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    Bolvar->TextEmote(SAY_BOLVAR2, Bolvar);
                Timer = 2000;
                break;
            case 65:
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                {
                    Bolvar->SetWalk(false);
                    Bolvar->GetMotionMaster()->MovePoint(0, WindsorEventMove[15]);
                       // MOVE_NONE, 0, 5.740616f);
                    X = Bolvar->GetPositionX() - WindsorEventMove[15].GetPositionX();
                    Y = Bolvar->GetPositionY() - WindsorEventMove[15].GetPositionY();
                    Timer = 1000 + sqrt((X * X) + (Y * Y)) / (me->GetSpeed(MOVE_WALK) * 0.001f);
                }
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                    Onyxia->Say(SAY_ONYXIA5);
                else Timer = 4000;
                break;
            case 66:
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    Bolvar->Say(SAY_BOLVAR3);
                break;
            case 67:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                    Onyxia->TextEmote(SAY_ONYXIA6, Onyxia);
                Timer = 2000;
                break;
            case 68:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                {
                    Onyxia->Say(SAY_ONYXIA7);
                    int Var = 0;
                    GetCreatureListWithEntryInGrid(DragListe, Onyxia, NPC_STORMWIND_ROYAL_GUARD, 25.0f);
                    for (const auto& itr : DragListe)
                    {
                        DragsGUIDs[Var] = itr->GetGUID();
                        itr->UpdateEntry(NPC_ONYXIA_ELITE_GUARD);
                        itr->AIM_Initialize();
                        itr->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        itr->SetReactState(REACT_DEFENSIVE);
                        if (!urand(0, 2))
                            itr->TextEmote(SAY_HISS, itr);
                        Var++;
                    }
                }
                Timer = 4000;
                break;
            case 69:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                    Onyxia->CastSpell(me, SPELL_WINDSOR_DEATH, false);
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    Bolvar->HandleEmoteCommand(EMOTE_STATE_READY_UNARMED);
                Timer = 1500;
                break;
            case 70:
                me->Say(SAY_WINDSOR20);
                me->SetUnitFlag2(UNIT_FLAG2_FEIGN_DEATH);
                Timer = 1000;
                break;
            case 71:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                {
                    Onyxia->Say(SAY_ONYXIA8);
                    if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    {
                        //Bolvar->SetFactionTemporary(FACTION_BOLVAR_COMBAT, TEMPFACTION_RESTORE_COMBAT_STOP); //TODO没明白干啥
                        Bolvar->SetFaction(FACTION_BOLVAR_COMBAT);
                        int Var = 0;
                        while (DragsGUIDs[Var] && Var < 9)
                        {
                            if (Creature* crea = me->GetMap()->GetCreature(DragsGUIDs[Var]))
                            {
                                crea->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                //仇恨相关
                                crea->GetThreatMgr().AddThreat(Bolvar, 5000.0f);
                                crea->SetTarget(Bolvar->GetGUID());
                                Bolvar->AddThreat(crea, 1.0f);
                                Bolvar->SetInCombatWith(crea);
                                crea->SetInCombatWith(Bolvar);
                            }
                            Var++;
                        }
                    }
                }
                Timer = 5000;
                break;
            case 72:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                {
                    Onyxia->Say(SAY_ONYXIA9);
                    Onyxia->CastSpell(Onyxia, SPELL_PRESTOR_DESPAWNS, true);
                }
                Timer = 1000;
                break;
            case 73:
                if (Creature* Onyxia = me->FindNearestCreature(NPC_LADY_ONYXIA, 150.0f))
                {
                    //Onyxia->ForcedDespawn();
                    Onyxia->DespawnOrUnsummon();
                    Onyxia->SetRespawnDelay(6 * MINUTE);
                    Onyxia->SetRespawnTime(6 * MINUTE);
                    Onyxia->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP);
                }
                Timer = 15000;
                break;
            case 74:
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                    Bolvar->TextEmote(SAY_BOVLAR4, Bolvar);
                break;
            case 75:
                PhaseFinale = true;
                Tick = 100; // come back when combat is done
                break;
            case 76:
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                {
                    Bolvar->HandleEmoteCommand(EMOTE_STATE_STAND);

                    Bolvar->Say(SAY_BOLVAR5);
                    Bolvar->SetWalk(true);
                    Bolvar->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    Bolvar->SetStandState(UNIT_STAND_STATE_KNEEL);
                }
                Timer = 1000;
                break;
            case 77:
                CompleteQuest();
                me->Say(SAY_WINDSOR21);
                Timer = 8000;
                break;
            case 78:
                me->TextEmote(SAY_WINDSOR22, me);
                //me->CastSpell(me, 7, true); // death
                EndScene();
                BeginQuest = false;
                break;
            }
            if (eventGardId < 6)
            {
                if (Creature* pGuard = GetGuard(eventGardId))
                {
                    int Var = eventGardId + 7;
                    pGuard->GetMotionMaster()->MovePoint(0, WindsorEventMove[Var]);
                    X = pGuard->GetPositionX() - WindsorEventMove[Var].GetPositionX();
                    Y = pGuard->GetPositionY() - WindsorEventMove[Var].GetPositionY();
                }
                GuardTimer[eventGardId] = 1000 + sqrt(X * X + Y * Y) / (me->GetSpeed(MOVE_WALK) * 0.001f);
                GuardNeed[eventGardId] = true;
                Timer = 1000;
            }
            if (Tick > 26 && Tick < 44)
            {
                int Var = Tick - 25;
                me->GetMotionMaster()->MovePoint(0, WindsorWaypoints[Var]);
                X = me->GetPositionX() - WindsorWaypoints[Var].GetPositionX();
                Y = me->GetPositionY() - WindsorWaypoints[Var].GetPositionY();
                Timer = 100 + sqrt(X * X + Y * Y) / (me->GetSpeed(MOVE_WALK) * 0.001f);
            }
            else if (Tick > 44 && Tick < 50)
            {
                int Var = Tick - 26;
                me->GetMotionMaster()->MovePoint(0, WindsorWaypoints[Var]);
                X = me->GetPositionX() - WindsorWaypoints[Var].GetPositionX();
                Y = me->GetPositionY() - WindsorWaypoints[Var].GetPositionY();
                Timer = 100 + sqrt(X * X + Y * Y) / (me->GetSpeed(MOVE_WALK) * 0.001f);
            }
            else if (PhaseFinale)
            {
                if (Creature* Bolvar = me->FindNearestCreature(NPC_BOLVAR_FORDRAGON, 150.0f))
                {
                    if (!Bolvar->IsInCombat())
                    {
                        if (!CombatJustEnded)
                        {
                            Bolvar->SetWalk(true);
                            Bolvar->GetMotionMaster()->MovePoint(0, { -8447.39f, 335.35f, 121.747f, 1.29f });
                            //Bolvar->ClearTemporaryFaction(); //TODO
                            CombatJustEnded = true;
                            PhaseFinale = false;
                            Timer = 5000;
                            Tick = 76;
                            return;
                        }
                    }
                }
            }
            Tick++;
        }
        else
            Timer -= uiDiff;
    }
};

struct npc_squire_roweAI : ScriptedAI
{
    npc_squire_roweAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    uint32 m_uiTimer;
    uint32 m_uiStep;
    bool m_bEventProcessed;
    bool m_bWindsorUp;
    ObjectGuid m_playerGuid;

    void Reset() override
    {
        m_playerGuid.Clear();
        m_uiTimer = 2000;
        m_uiStep = 0;
        m_bEventProcessed = false;
        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        m_bWindsorUp = false;
    }
    //void ResetCreature() override; //az没有这个函数了，对应代码在reset里，有待测试
    void MovementInform(uint32 uiType, uint32 uiPointId) override
    {
        if (!m_bEventProcessed || uiType != POINT_MOTION_TYPE)
            return;

        switch (uiPointId)
        {
        case 1:
            me->GetMotionMaster()->MovePoint(2, RoweWaypoints[0]);
            break;
        case 2:
            me->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
            m_uiTimer = 5000;
            ++m_uiStep;
            break;
        case 4:
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            Talk(1);
            m_bEventProcessed = false;
            break;
        }
    }
    void UpdateAI(uint32 const uiDiff) override
    {
        if (m_bEventProcessed)
        {
            if (m_uiTimer < uiDiff)
            {
                switch (m_uiStep)
                {
                case 0:
                    me->SetSpeedRate(MOVE_RUN, 1.1f);
                    me->GetMotionMaster()->MovePoint(1, RoweWaypoints[1]);
                    m_uiTimer = 1000;
                    ++m_uiStep;
                    break;
                case 2:
                    //todo 删除召唤物
                    me->SummonGameObject(GO_FLARE_OF_JUSTICE, -9095.839844f, 411.178986f, 92.244499f, 2.303830f, 0.0f, 0.0f, 0.913545f, 0.406738f, 10 * IN_MILLISECONDS);
                    me->GetMotionMaster()->MovePoint(3, RoweWaypoints[0]);
                    m_uiTimer = 1500;
                    ++m_uiStep;
                    break;
                case 3:
                    if (Creature* pWindsor = me->SummonCreature(NPC_REGINALD_WINDSOR,
                        WindsorSummon, TEMPSUMMON_MANUAL_DESPAWN, 1.5 * HOUR * IN_MILLISECONDS, true))
                    {
                        npc_reginald_windsorAI* pWindsorAI = CAST_AI(npc_reginald_windsorAI,pWindsor->AI());

                        if (pWindsorAI)
                        {
                            if (m_playerGuid)
                                pWindsorAI->playerGUID = m_playerGuid;

                            pWindsorAI->m_squireRoweGuid = me->GetGUID();
                        }

                        m_bWindsorUp = true;
                        pWindsor->Mount(MOUNT_WINDSOR);
                        pWindsor->SetWalk(false);
                        pWindsor->SetSpeedRate(MOVE_RUN, 1.0f);
                        pWindsor->GetMotionMaster()->MovePoint(0, WindsorWaypoints[0]);
                        pWindsor->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    }
                    ++m_uiStep;
                    break;
                case 4:
                    float x, y, z, o;
                    me->GetRespawnPosition(x, y, z, &o);
                    me->GetMotionMaster()->MovePoint(4, { x, y, z , o });
                    //清楚召唤信号
                    if (GameObject* go = me->FindNearestGameObject(GO_FLARE_OF_JUSTICE, 150.0f))
                        go->DespawnOrUnsummon();
                    ++m_uiStep;
                    break;
                }
            }
            else
                m_uiTimer -= uiDiff;
        }
        else
            ScriptedAI::UpdateAI(uiDiff);
    }
};


void npc_reginald_windsorAI::PokeRowe()
{
    if (!m_bRoweKnows)
    {
        // let Squire Rowe know he can allow players to proceed with quest now
        if (auto pRowe = me->GetMap()->GetCreature(m_squireRoweGuid))
        {
            if (auto pRoweAI = static_cast<npc_squire_roweAI*>(pRowe->AI()))
            {
                pRoweAI->Reset();
                m_bRoweKnows = true;
            }
        }
    }
}

class npc_reginald_windsor : public CreatureScript
{
public:
    npc_reginald_windsor() :CreatureScript("npc_reginald_windsor") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_THE_GREAT_MASQUERADE)
        {
            if (auto pWindsorEventAI = dynamic_cast<npc_reginald_windsorAI*>(creature->AI()))
            {
                pWindsorEventAI->BeginQuest = true;
                pWindsorEventAI->QuestAccepted = true;
                pWindsorEventAI->GreetPlayer = false;
                pWindsorEventAI->playerGUID = player->GetGUID();
                pWindsorEventAI->m_uiDespawnTimer = 30 * MINUTE * IN_MILLISECONDS;
                creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            }
        }
        return true;
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        ClearGossipMenuFor(pPlayer);
        if (auto pWindsorEventAI = static_cast<npc_reginald_windsorAI*>(pCreature->AI()))
        {
            if (pPlayer == pWindsorEventAI->GetPlayer() && pWindsorEventAI->QuestAccepted)
            {
                AddGossipItemFor(pPlayer, GOSSIP_ICON_CHAT, "我准备好了，我的军队也整装待命。让我们结束这次假面舞会吧！", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            }
            else if (pCreature->IsQuestGiver())
            {
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());
            }
            SendGossipMenuFor(pPlayer, 5633, pCreature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF:
            if (auto pWindsorEventAI = static_cast<npc_reginald_windsorAI*>(creature->AI()))
            {
                pWindsorEventAI->BeginQuest = true;
                creature->Say(SAY_WINDSOR12);
                creature->SetUInt32Value(UNIT_NPC_FLAGS, 0);
            }
            CloseGossipMenuFor(player);
            break;
        }
        return true;
    }
    
    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_reginald_windsorAI(creature);
    }
};

class npc_squire_rowe : public CreatureScript
{
public:
    npc_squire_rowe():CreatureScript("npc_squire_rowe") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        ClearGossipMenuFor(pPlayer);
        if ((pPlayer->GetQuestStatus(QUEST_STORMWIND_RENDEZVOUS) == QUEST_STATUS_COMPLETE || pPlayer->GetQuestStatus(QUEST_STORMWIND_RENDEZVOUS) == QUEST_STATUS_REWARDED) &&
            pPlayer->GetQuestStatus(QUEST_THE_GREAT_MASQUERADE) != QUEST_STATUS_COMPLETE)
        {
            auto pSquireRoweAI = static_cast<npc_squire_roweAI*>(pCreature->AI());

            if (!pSquireRoweAI)
                return true;

            if (!pSquireRoweAI->m_bWindsorUp)
            {
                AddGossipItemFor(pPlayer, GOSSIP_ICON_CHAT, "请转告温德索尔元帅我已准备就绪。", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                SendGossipMenuFor(pPlayer, GOSSIP_ROWE_READY, pCreature->GetGUID());
            }
            else
                SendGossipMenuFor(pPlayer, GOSSIP_ROWE_BUSY, pCreature->GetGUID());
        }
        else if (pPlayer->GetQuestStatus(QUEST_THE_GREAT_MASQUERADE) == QUEST_STATUS_COMPLETE)
            SendGossipMenuFor(pPlayer, GOSSIP_ROWE_COMPLETED, pCreature->GetGUID());
        else
            SendGossipMenuFor(pPlayer, GOSSIP_ROWE_NOTHING, pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF:
            if ((player->GetQuestStatus(QUEST_STORMWIND_RENDEZVOUS) == QUEST_STATUS_COMPLETE || player->GetQuestStatus(QUEST_STORMWIND_RENDEZVOUS) == QUEST_STATUS_REWARDED) &&
                player->GetQuestStatus(QUEST_THE_GREAT_MASQUERADE) != QUEST_STATUS_COMPLETE)
            {
                if (auto pSquireRoweAI = static_cast<npc_squire_roweAI*>(creature->AI()))
                {
                    pSquireRoweAI->m_bEventProcessed = true;
                    pSquireRoweAI->m_playerGuid = player->GetGUID();
                    creature->SetWalk(false);
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }
            }

            CloseGossipMenuFor(player);
            break;
        }

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_squire_roweAI(creature);
    }
};

void AddSC_stormwind_city()
{
    new npc_bartleby();
    new npc_tyrion();
    new npc_tyrion_spybot();
    new npc_lord_gregor_lescovar();
    new npc_marzon_silent_blade();
    new npc_squire_rowe();
    new npc_reginald_windsor();
}
