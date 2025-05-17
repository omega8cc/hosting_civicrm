<?php
/**
 * @file
 * Expose the civiplatform feature to hostmaster.
 */

/**
 * Implements hook_hosting_feature().
 */
function hosting_civiplatform_hosting_feature() {
  $features['civiplatform'] = array(
    'title' => t('Platform administration'),
    'description' => t('Create and manage civiplatforms.'),
    'status' => HOSTING_FEATURE_REQUIRED,
    'module' => 'hosting_civiplatform',
    'node' => 'civiplatform',
    'group' => 'required',
    'role_permissions' => array(
      'aegir civiplatform manager' => array(
        'administer civiplatforms',
        'create civiplatform',
        'delete civiplatform',
        'edit civiplatform',
        'view locked civiplatforms',
        'view civiplatform',
        'create sites on locked civiplatforms',
      ),
      'aegir client' => array(
        'view civiplatform',
      ),
    ),

  );
  return $features;
}
