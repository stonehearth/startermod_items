local constants = require 'constants'

local UpgradeEquipmentPiece = class()
UpgradeEquipmentPiece.name = 'upgrade equipment'
UpgradeEquipmentPiece.does = 'stonehearth:work'
UpgradeEquipmentPiece.args = {}
UpgradeEquipmentPiece.version = 2
UpgradeEquipmentPiece.priority = constants.priorities.work.UPGRADE_EQUIPMENT

local ai = stonehearth.ai
return ai:create_compound_action(UpgradeEquipmentPiece)
         :execute('stonehearth:find_equipment_upgrade')
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:pickup_item', {
            item = ai.BACK(2).item,
         })
         :execute('stonehearth:equip_carrying')
