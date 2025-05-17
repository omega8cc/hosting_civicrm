<?php
/**
 * @file
 * Expose the civisite feature to hostmaster.
 */

/**
 * Implements hook_hosting_feature().
 */
function hosting_civisite_hosting_feature() {
  $features['civisite'] = array(
    'title' => t('Wordpress site administration'),
    'description' => t('Create and manage CiviCRM sites.'),
    'status' => HOSTING_FEATURE_DISABLED,
    'module' => 'hosting_civisite',
    'node' => 'civisite',
    'group' => 'experimental',
    'role_permissions' => array(
      'aegir platform manager' => array(
        'administer civisites',
        'create civisite',
        'delete civisite',
        'edit civisite',
        'view civisite',
      ),
      'aegir account manager' => array(
        'view civisite',
      ),
      'aegir client' => array(
        'create civisite',
        'delete civisite',
        'view civisite',
        'edit civisite',
      ),
    ),
  );
  return $features;
}
