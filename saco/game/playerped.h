
#pragma once

#include "game.h"
#include "aimstuff.h"
#include "entity.h"

struct struc_97
{
	char _gap0[52];
};

//-----------------------------------------------------------

class CPlayerPed : public CEntity
{
public:

	void ResetPointers();
	void SetInitialState();

	void  SetKeys(WORD wKeys, WORD lrAnalog, WORD udAnalog);
	WORD  GetKeys(WORD * lrAnalog, WORD * udAnalog);

	BYTE GetSpecialKey();

	CAMERA_AIM * GetCurrentAim();
	void SetCurrentAim(CAMERA_AIM *pAim);

	BYTE GetCameraMode() {
		if(!m_bytePlayerNumber)
			return GameGetLocalPlayerCameraMode();
		else
			return GameGetPlayerCameraMode(m_bytePlayerNumber);
	};

	void  ShowMarker(int iMarkerColorID);
	void  HideMarker();
	BYTE  GetCurrentWeapon();
	int   GetCurrentVehicleID();
	BOOL  IsOnScreen();
	float GetHealth();
	void  SetHealth(float fHealth);
	float GetArmour();
	void  SetArmour(float fArmour);
	DWORD GetStateFlags();
	void  SetStateFlags(DWORD dwStateFlags);
	BOOL  IsDead();
	BOOL  IsInVehicle();
	BYTE  GetActionTrigger();
	void  SetActionTrigger(BYTE byteTrigger);

	WORD  GetAmmo();

	int   GetFightingStyle();
	void  SetFightingStyle(int iStyle);

	float GetTargetRotation();
	void  SetTargetRotation(float fRotation);
	void  ForceTargetRotation(float fRotation);

	void GiveWeapon(int iWeaponID, int iAmmo);
	void SetArmedWeapon(int iWeaponType, bool bUnk);
	void ClearAllWeapons();
	void RemoveWeaponWhenEnteringVehicle();
	WEAPON_SLOT_TYPE * GetCurrentWeaponSlot();
	WEAPON_SLOT_TYPE * FindWeaponSlot(DWORD dwWeapon);
	BOOL HasAmmoForCurrentWeapon();

	void SetImmunities(BOOL bBullet, BOOL bFire, BOOL bExplosion, BOOL bDamage, BOOL bUnknown);

	void PutDirectlyInVehicle(int iVehicleID, int iSeat);
	void EnterVehicle(int iVehicleID, BOOL bPassenger);
	void ExitCurrentVehicle();
	void RemoveFromVehicleAndPutAt(float fX, float fY, float fZ);

	BOOL IsAPassenger();

	VEHICLE_TYPE * GetGtaVehicle();
	VEHICLE_TYPE * GetGtaContactVehicle();
	ENTITY_TYPE * GetGtaContactEntity();

	int GetVehicleSeatID();
	void TogglePlayerControllable(int iControllable);

	float GetDistanceFromVehicle(CVehicle *pVehicle);

	void StartJetpack();
	void StopJetpack();
	BOOL IsInJetpackMode();

	void StartGoggles();
	void StopGoggles();
	BOOL HasGoggles();

	void SetAnimationSet(PCHAR szAnim);
	void SetMoney(int iAmount);
	void ApplyAnimation(char *szAnimName, char *szAnimFile, float fT,
						int opt1, int opt2, int opt3, int opt4, int iUnk);

	BOOL IsPerformingAnimation(char *szAnimName);

	CPlayerPed();

	void		ProcessVehicleHorn();

	void		StartDancing(int iStyle);
	void		StopDancing();
	BOOL		IsDancing();
	void		HandsUp();
	BOOL		HasHandsUp();
	void		HoldItem(int iObject);

	void		ProcessMarkers(BOOL bMarkerStreamingEnabled, float fMarkerStreamRadius, BOOL bVisible);

	void		ApplyCommandTask(char *szTaskName, int p1, int p2, int p3, 
								VECTOR *p4, int p5, float p6, int p7, int p8, int p9);

	void		DestroyFollowPedTask();
	void		ToggleCellphone(int iOn);
    int			IsCellphoneEnabled();

	BOOL		IsJumping();
	BOOL		IsFighting();
	BOOL		IsFalling();
	BOOL		IsSwimming();

	float		GetAimZ();
	void		SetAimZ(float fAimZ);

	PCHAR		GetLoadedShopName();
	void		LoadShoppingDataSubsection(PCHAR szSectionName);
	PCHAR		GetShopName();
	void		SetShopName(char *szNewShopName);

	void		SetWeaponSkillLevel(int iSkill, WORD wLevel);

	VECTOR*		GetBonePosition(int iBone, VECTOR *vecPos);
	VECTOR*		GetTransformedBonePosition(int iBone, VECTOR *vecOffset);

	int			m_iCellPhoneEnabled;
	int field_4C[10];
	struc_97 field_74[10];
	int field_27C[10];
	PED_TYPE    *m_pPed;
	int field_2A8;
	int field_2AC;
	BYTE		m_bytePlayerNumber;
	char _gap2B1[8];
	int field_2B9;
	char _gap2BD[4];
	int field_2C1;
	int field_2C5;
	int field_2C9;
	DWORD 		m_dwArrow;
	char _gap2D1;
	int			m_iDanceState;
	int			m_iDanceStyle;
	char _gap2DA[4];
	int field_2DE;
	int field_2E2;
	BOOL		m_bGoggleState;
	char _gap2EA[8];
	int field_2F2;
	char field_2F6;
	char field_2F7;
	char field_2F8;
	int field_2F9;
	char _gap2FD[48];

};

//-----------------------------------------------------------
