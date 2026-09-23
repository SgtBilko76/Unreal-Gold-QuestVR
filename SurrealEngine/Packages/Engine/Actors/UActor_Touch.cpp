
#include "Precomp.h"
#include "UActor.h"
#include "VM/ScriptCall.h"
#include "Engine.h"

void UActor::CheckPendingTouch()
{
	if (engine->LaunchInfo.ue1Version >= 400)
	{
		if (PendingTouch())
		{
			CallEvent(PendingTouch(), EventName::PostTouch, { ExpressionValue::ObjectValue(this) });
			if (PendingTouch())
			{
				UActor* cur = PendingTouch();
				UActor* next = cur->PendingTouch();
				PendingTouch() = next;
				cur->PendingTouch() = nullptr;
			}
		}
	}
}

void UActor::Touch(UActor* actor)
{
	// Don't setup touch if any object has been destroyed
	if (bDeleteMe() || actor->bDeleteMe())
		return;

	if (PropOffsets_Actor.TouchingIsDynamicArray)
	{
		// 469 turned Actor.Touching from a fixed [4] into a dynamic array (unlimited touches).
		// It starts EMPTY, so the previous version of this branch - which only ever looked for
		// an existing null slot and gave up when it found none - never linked anything and no
		// Touch event was ever sent (the "touch events don't work on 469" note in
		// Docs/Status.md: no pickups, no triggers, no teleporters). Grow the array instead.
		auto TouchingArray = Touching_UT469();
		auto TouchingArray2 = actor->Touching_UT469();

		// Do nothing if actors are already touching
		for (UActor* touching : TouchingArray)
		{
			if (touching == actor)
				return;
		}

		// Setup links first so Destroy or recursive Touch calls always find the touch binding.
		// Null slots (left behind by UnTouch, which never shifts entries - see its comment)
		// are reused before growing.
		auto link = [](TypedScriptArray<UActor*>& array, UActor* value)
		{
			for (UActor*& slot : array)
			{
				if (slot == nullptr)
				{
					slot = value;
					return;
				}
			}
			array.push_back(value);
		};
		link(TouchingArray, actor);
		link(TouchingArray2, this);

		// Notify unrealscript for first actor
		CallEvent(this, EventName::Touch, { ExpressionValue::ObjectValue(actor) });

		// Notify unrealscript for second actor - unless the first handler already untouched or
		// destroyed either side (the equivalent of the static-array path's TouchEventSent
		// bookkeeping: re-read the live array rather than trusting stale state).
		if (!bDeleteMe() && !actor->bDeleteMe())
		{
			for (UActor* touching : actor->Touching_UT469())
			{
				if (touching == this)
				{
					CallEvent(actor, EventName::Touch, { ExpressionValue::ObjectValue(this) });
					break;
				}
			}
		}
	}
	else
	{
		auto TouchingArray = Touching();
		auto TouchingArray2 = actor->Touching();

		// Do nothing if actors are already touching
		for (int i = 0; i < TouchingArraySize; i++)
		{
			if (TouchingArray[i] == actor)
				return;
		}

		// Only setup touch if we have room in both arrays
		int slot1 = -1, slot2 = -1;
		for (int i = 0; i < TouchingArraySize; i++)
		{
			if (slot1 == -1 && TouchingArray[i] == nullptr)
				slot1 = i;
			if (slot2 == -1 && TouchingArray2[i] == nullptr)
				slot2 = i;
		}
		if (slot1 == -1 || slot2 == -1)
			return;

		// Setup links first so Destroy or recursive Touch calls always finds the touch binding
		TouchingArray[slot1] = actor;
		TouchEventSent[slot1] = true;
		TouchingArray2[slot2] = this;
		actor->TouchEventSent[slot2] = false;

		// Notify unrealscript for first actor
		CallEvent(this, EventName::Touch, { ExpressionValue::ObjectValue(actor) });

		// Notify unrealscript for second actor
		if (!actor->bDeleteMe())
		{
			for (int i = 0; i < TouchingArraySize; i++)
			{
				if (TouchingArray2[i] == this && !actor->TouchEventSent[i])
				{
					actor->TouchEventSent[i] = true;
					CallEvent(actor, EventName::Touch, { ExpressionValue::ObjectValue(this) });
					break;
				}
			}
		}
	}
}

void UActor::UnTouch(UActor* actor)
{
	if (PropOffsets_Actor.TouchingIsDynamicArray)
	{
		// Dynamic-array counterpart of the static path below. Entries are nulled, NOT erased:
		// every caller (UActor::Destroy, UActor_Phys's overlap sweeps) range-iterates the live
		// Touching array while calling this, so shifting elements would skip entries and walk
		// past a stale end pointer. Trailing nulls are trimmed so the array doesn't stay
		// inflated (and so script code walking Touching.Length sees as few None entries as
		// possible - it already has to tolerate them, as the [4] version had None slots too).
		auto unlink = [](TypedScriptArray<UActor*>& array, UActor* value) -> bool
		{
			bool found = false;
			for (UActor*& slot : array)
			{
				if (slot == value)
				{
					slot = nullptr;
					found = true;
				}
			}
			while (array.size() > 0 && array[array.size() - 1] == nullptr)
				array.pop_back();
			return found;
		};

		auto TouchingArray = Touching_UT469();
		if (unlink(TouchingArray, actor) && !bDeleteMe())
			CallEvent(this, EventName::UnTouch, { ExpressionValue::ObjectValue(actor) });

		auto TouchingArray2 = actor->Touching_UT469();
		if (unlink(TouchingArray2, this) && !actor->bDeleteMe())
			CallEvent(actor, EventName::UnTouch, { ExpressionValue::ObjectValue(this) });
		return;
	}

	auto TouchingArray = Touching();
	auto TouchingArray2 = actor->Touching();

	if (!bDeleteMe())
	{
		for (int i = 0; i < TouchingArraySize; i++)
		{
			if (TouchingArray[i] == actor)
			{
				TouchingArray[i] = nullptr;
				if (TouchEventSent[i])
				{
					TouchEventSent[i] = false;
					CallEvent(this, EventName::UnTouch, { ExpressionValue::ObjectValue(actor) });
				}
			}
		}
	}

	if (!actor->bDeleteMe())
	{
		for (int i = 0; i < TouchingArraySize; i++)
		{
			if (TouchingArray2[i] == this)
			{
				TouchingArray2[i] = nullptr;
				if (actor->TouchEventSent[i])
				{
					actor->TouchEventSent[i] = false;
					CallEvent(actor, EventName::UnTouch, { ExpressionValue::ObjectValue(this) });
				}
			}
		}
	}
}
