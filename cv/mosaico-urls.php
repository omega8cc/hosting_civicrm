<?php

// This should be invoked with: cv php:script cv/mosaico-urls.php <old host> <new host>
// Mosaico templates and the mailings made from them keep absolute URLs: point
// them at the new host after a clone or a rename. The same rewrite the Drupal
// deploy hook runs in process, for sites where it runs inside cv.

$old = isset($argv[1]) ? $argv[1] : '';
$new = isset($argv[2]) ? $argv[2] : '';

if ($old === '' || $new === '' || $old === $new) {
  echo json_encode(['is_error' => 0, 'values' => 'nothing to rewrite']);
  return;
}

if (!CRM_Core_DAO::checkTableExists('civicrm_mosaico_template')) {
  echo json_encode(['is_error' => 0, 'values' => 'no Mosaico tables']);
  return;
}

CRM_Core_DAO::executeQuery('UPDATE civicrm_mosaico_template SET metadata = REPLACE(metadata, %1, %2)', [
  1 => [$old, 'String'],
  2 => [$new, 'String'],
]);

// Required so that re-using old mailings still works
CRM_Core_DAO::executeQuery('UPDATE civicrm_mailing SET template_options = REPLACE(template_options, %1, %2)', [
  1 => [$old, 'String'],
  2 => [$new, 'String'],
]);

echo json_encode(['is_error' => 0, 'values' => 'rewritten']);
